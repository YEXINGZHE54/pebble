#include<event2/event.h>
#include<fcgiapp.h>
#include<pthread.h>
#include "notifier.h"
#include "event.h"
#include "opool.h"
#include "threadinfo.h"
#include "framework.h"
#include "app.h"

int async_opool_init(void **ptr, opool_t *p)
{
    *ptr = ngx_palloc(p->mem, sizeof(stone_async_t));
    if(*ptr == NULL) return -1;
    return 0;
}

void async_thread_destroy(void *d)
{
    stone_async_thread *a = d;
    if(!a) return;
    if(a->back) ngx_destroy_pool(a->back);
    if(a->use) ngx_destroy_pool(a->use);
    if(a->que) opool_destroy(a->que);
    pthread_mutex_destroy(&a->lock); //unlock if exit
    if(a->pool) ngx_destroy_pool(a->pool);
    return;
}

int async_thread_init(void *d)
{
    struct GLOBAL_RAPHTERS *g = d;
    stone_async_thread *a;
    a = ngx_palloc(g->pool, sizeof(*a));
    if(!a) goto failed;
    a->pThread = ngx_palloc(g->pool, sizeof(pthread_t));
    if(!a->pThread) goto failed;
    a->pool = ngx_create_pool(4096, g->log);
    if(!a->pool) goto failed;
    a->que = opool_create(a->pool, OPOOL_FLAG_RESIZE, 0, async_opool_init, NULL);
    if(!a->que) goto failed;
    a->use = ngx_create_pool(4096, g->log);
    if(!a->use) goto failed;
    a->back = ngx_create_pool(4096, g->log);
    if(!a->back) goto failed;
    pthread_mutex_init(&a->lock, NULL);
    g->async = a;
    return 0;
failed:
    async_thread_destroy(a);
    return -1;
}

//async thread start here
void * async_thread_start(void *d)
{
    stone_async_thread *a = d;
    opool_iter *iter, *head;
    stone_async_t *a_t;
    void *temp;
    pthread_cleanup_push(async_thread_destroy, a);
    while(1){
        pthread_mutex_lock(&a->lock);
        iter = opool_requestAll(a->que);
        if(iter == NULL){
            pthread_mutex_unlock(&a->lock);
            sleep(60);
            continue;
        }
        //exchange use and back
        temp = a->use;
        a->use = a->back;
        a->back  = temp;
        pthread_mutex_unlock(&a->lock);
        head = iter;//store head to release
        //now deal with que
        while(iter){
            a_t = iter->data;
            a_t->fn(a_t->fd, a_t->data);
            iter = iter->next;
        }
        //after all, reset pool and return all to que
        ngx_reset_pool(a->back);
        opool_releaseAll(a->que, head);
    }
    pthread_cleanup_pop(0);
}

int async_delegate(int fd, void *data, async_callfn fn, async_setfn datafn)
{
    stone_async_thread *a = globals_r.async;
    if(!a) return -1;
    pthread_mutex_lock(&a->lock);
    stone_async_t *at = opool_request(a->que);
    if(!at){
        pthread_mutex_unlock(&a->lock);
        return -1;
    }
    at->fd = fd;
    at->data = data;
    at->fn = fn;
    at->pool = a->use;
    if(datafn) datafn(at);
    opool_release(a->que, at);
    pthread_mutex_unlock(&a->lock);
    return 0;
}

void response_handler(int fd, short which, void *d)
{
    stone_event *ev = d;
    stone_server_t *server = ev->data;
    triger(NOTIFIER_RESPONSE, server);
	if(server->fcgx)
    {
        FCGX_Finish_r( server->fcgx );
        opool_release(server->thread->connections, server->fcgx);
    }
    event_del(ev->ev);
    opool_release(server->thread->ev, ev);
    stone_app_stop(server);
    return;
}

//inside current thread
int async_register(stone_server_t *s, int fd, int flag, event_callback_fn callback_fn){
    stone_event *ev;
    ev = opool_request(s->thread->ev);
    if(!ev) return -1;
    s->async += 1;
    ev->data = s;
    event_assign(ev->ev, s->thread->base, fd, flag, callback_fn, ev);
    event_add(ev->ev, 0);
    return 0;
}

void async_schedule_do(stone_server_t *s)
{
    stone_event *ev;
    // leave async ev to be released in response_handler
    ev = opool_request(s->thread->ev);
    if(!ev) return;
    ev->data = s;
    event_assign(ev->ev, s->thread->base, s->fcgx->ipcFd, EV_WRITE|EV_PERSIST, response_handler, ev);
    event_add(ev->ev, 0);
    return;
}

void async_schedule(void *d)
{
    stone_event *ev = d;
    stone_server_t *server = ev->data;
    event_del(ev->ev);
    server->async -= 1;
    // release async ev now
    opool_release(server->thread->ev, ev);
    if(server->async == 0 ){
        async_schedule_do(server);
    }else if(server->async <0 ){
        // error!
    }
    return;
}

void request_handler(int fd, short which, void *d)
{
    handler *cur;
	stone_server_t *server = NULL;
	int method, rc, result = 0;
    struct FCGX_Request *fcgx = NULL; 
    stone_event *ev = d;
    struct thread_info_t *threadInfo = ev->data;
    fcgx = opool_request(threadInfo->connections);
    if(!fcgx) goto failure;
    FCGX_InitRequest ( fcgx, 0, 0 );
    fcgx->ipcFd = fd;
    fcgx->keepConnection = 1;
    result = FCGX_Accept_r( fcgx );
    if(result != 0) goto failure;
    fcgx->keepConnection = 0;
	char *path_info = FCGX_GetParam("PATH_INFO", fcgx->envp);
    if (path_info == NULL) goto failure;
	char *method_str = FCGX_GetParam("REQUEST_METHOD", fcgx->envp);
    if (method_str == NULL) goto failure;
    if (strcmp(method_str, "GET") == 0) {
        method = GET;
    } else if (strcmp(method_str, "POST") == 0) {
        method = POST;
    } else if (strcmp(method_str, "PUT") == 0) {
        method = PUT;
    } else if (strcmp(method_str, "HEAD") == 0) {
        method = HEAD;
    } else if (strcmp(method_str, "DELETE") == 0) {
        method = DELETE;
    } else {
        goto failure;
    }
		for (cur = get_handler_head(); cur != NULL; cur = cur->next)
        {
			if (cur->method == method) {
				regmatch_t *matches = ngx_palloc(threadInfo->reuse, sizeof(regmatch_t) * (cur->nmatch));
				int m = regexec(&cur->regex, path_info, cur->nmatch, matches, 0);
				if (m == 0) {
					server = stone_app_start(threadInfo, fcgx);
					store_path_info ( server, path_info, matches, cur->nmatch );
					cur->func(server);
					goto success;
				}
			}
        }


failure:
    //if(fcgx) error_handler("error!", fcgx);
success:
	//if(fcgx) {FCGX_Finish_r( fcgx );opool_release(threadInfo->connections, fcgx);}
    event_del(ev->ev);
    opool_release(threadInfo->ev, ev);
    if(server){
        // now not to write; wait for async event finishes unless async is 0
        if(server->async == 0 ){
            async_schedule_do(server);
        } 
    }else{
        if(fcgx) {opool_release(threadInfo->connections, fcgx);}
    }
    ngx_reset_pool(threadInfo->reuse);
    return;
}

void thread_pipe_handler (int fd, short which, void *d){
    char buf[1];
    int accept_sock;
    stone_event *ev;
    struct thread_info_t *threadInfo;

    if(read(fd, buf, 1) != 1){
        //log here
        return;
    }
    ev = d;
    threadInfo = ev->data;

    accept_sock = (int)opool_request(threadInfo->con_que);
    if(accept_sock <= 0) return;
    ev = opool_request(threadInfo->ev);
    if(!ev) return;
    ev->data = (void*)threadInfo;
    event_assign(ev->ev, threadInfo->base, accept_sock, EV_READ|EV_PERSIST, request_handler, ev);
    event_add(ev->ev, 0);
    return;
    //dispatch_thread_que((struct thread_info_t *)d);
}
