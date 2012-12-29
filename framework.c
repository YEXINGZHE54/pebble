#include "framework.h"
#include "notifier.h"
#include "threadinfo.h"
#include "error.h"
#include "event.h"
#include "async.h"
#include <time.h>
#include <event2/event.h>
#include <sys/socket.h>

#define nThreads 4
//static pthread_mutex_t PTHREAD_MUTEX_INITIALIZER;

void atquit(void){
	app_close(0);
}

void atquit_thread(void *d){
    struct thread_info_t *t = d;
    if(t->pool) ngx_destroy_pool(t->pool);
    if(t->reuse) ngx_destroy_pool(t->reuse);
    triger(NOTIFIER_THREAD_EXIT, t);
}

void dispatch(ngx_pool_t *pool){}

void worker_process(void *data){
	ngx_pool_t *pool;
	pool = ngx_create_pool(4096, globals_r.log);
	if (pool == NULL){
			//error_handler("Error while creating pool buffer!");
			return;
	}

    while(FCGI_Accept() >= 0) {
		ngx_reset_pool(pool);
        dispatch(pool);
    }
}

int spawn_worker(void *data, void (*proc)(void *)){
    int pid = fork();
    switch(pid){
        case -1:
            printf("%s","error fork!");
            return -1;
        case 0:
            printf("now pid is wrong? %d", getpid());
            (*proc)(data);
            break;
        default:
            printf("whereis it? %d", pid);
            break;
    }
    return pid;
}

/*
void dispatch_thread_que(struct thread_info_t *threadInfo) {
    handler *cur;
	//stone_response_t *res;
	stone_server_t *server;
	ngx_pool_t *pool = threadInfo->reuse;
	ngx_command_t command;
// 	unsigned int i;
	int method, rc, accept_sock = 0, result = 0;
    struct FCGX_Request *fcgx = NULL; 
    //stone_conn *con = NULL;

    /*
    pthread_mutex_lock(&threadInfo->con_que.lock);
    if(list_empty(&threadInfo->con_que.head)) goto release_lock;
    con = list_first_entry(&threadInfo->con_que.head, stone_conn, head);
    if(!con) goto release_lock;
    fcgx = &con->fcgx;
    list_del(&con->head);
release_lock:
    pthread_mutex_unlock(&threadInfo->con_que.lock);

    if(!fcgx) goto failure;
    //
    fcgx = opool_request(threadInfo->connections);
    if(!fcgx) goto failure;

    FCGX_InitRequest ( fcgx, 0, 0 );
    fcgx->ipcFd = accept_sock;
    fcgx->keepConnection = 1;
    result = FCGX_Accept_r( fcgx );
    if(result != 0) goto failure;
    fcgx->keepConnection = 0;

	char *path_info = FCGX_GetParam("PATH_INFO", fcgx->envp);
    if (path_info == NULL) {
        //error_handler("NULL path_info", fcgx);
        //return;
        goto failure;
    }
   //getenv("REQUEST_METHOD");
	 char *method_str = FCGX_GetParam("REQUEST_METHOD", fcgx->envp);
    if (method_str == NULL) {
        goto failure;
        //error_handler("NULL method_str", fcgx);
        //return;
    }

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
        //error_handler("unknown request method", fcgx);
        //return;
    }

// 					command.pool = server->pool;
					command.policy = &redis_cache_cmd;
					command.resource = threadInfo->redis;
					command.data = NULL;

		//i = 0;
        //while( app_handlers[i].func != 0 )
		for (cur = get_handler_head(); cur != NULL; cur = cur->next)
        {
			//cur = app_handlers + i;
			if (cur->method == method) {
				regmatch_t *matches = ngx_palloc(pool, sizeof(regmatch_t) * (cur->nmatch));
				int m = regexec(&cur->regex, path_info, cur->nmatch, matches, 0);
				if (m == 0) {
					server = stone_app_start(threadInfo, fcgx);
					command.pool = server->pool;
					store_path_info ( server, path_info, matches, cur->nmatch );
					/*rc = session_init ( server, &command );
					if ( rc != SESSION_OK ) {
						goto failure;
					}//
					cur->func(server);
					//session_save ( server, &command );
					//response_send(server->res);
                    triger(NOTIFIER_RESPONSE, server);
					stone_app_stop(server);
					//ngx_pfree(matches);
					goto success;
				}
				//free(matches);
				//ngx_pfree(pool, matches);
			}
            //i = i + 1;  // Next function
        }
	failure:
        if(fcgx) error_handler("error occured!", fcgx);
    success:
		if(fcgx) {FCGX_Finish_r( fcgx );opool_release(threadInfo->connections, fcgx);}
		ngx_reset_pool(pool);
		//ngx_destroy_pool(pool);
        //if(con) free(con);
		return;
}
*/

void dispatch_thread(struct thread_info_t *d){}

extern void thread_pipe_handler(int, short, void *);

void * worker_thread(void *data){
	struct thread_info_t *threadInfo = data;
	pthread_mutex_t *accept_mutex = threadInfo->lock;
	ngx_pool_t *pool, *reuse;
    // add cleanup hook
    pthread_cleanup_push(atquit_thread, threadInfo);
	pool = ngx_create_pool(4096, globals_r.log);
	if (pool == NULL){
			perror("Error while creating pool buffer!");
			return;
	}
	threadInfo->pool = pool;
    reuse = ngx_create_pool(4096, globals_r.log);
    if(reuse == NULL) return;
    threadInfo->reuse = reuse;
    triger(NOTIFIER_THREAD, threadInfo);
    /*
	while(1){
		pthread_mutex_lock( accept_mutex );
        int result = FCGX_Accept_r( fcgx );
        pthread_mutex_unlock( accept_mutex );
        if(result != 0) continue;
		dispatch_thread(threadInfo);
		FCGX_Finish_r( fcgx );
		ngx_reset_pool(threadInfo->pool);
	}
    */
    stone_event *ev = opool_request(threadInfo->ev);
    if(!ev) return;
    ev->data = threadInfo;
    event_assign(ev->ev, threadInfo->base, threadInfo->receive_pipe, EV_READ | EV_PERSIST, thread_pipe_handler, ev);
    event_add(ev->ev, 0);
    event_base_dispatch(threadInfo->base);
    pthread_cleanup_pop(0);
}

void stone_run(void){
    int i,rc;
	app_init();
	FCGX_Init();
	struct thread_info_t *threadInfo;
	struct thread_info_t **pThreadList;
    pthread_mutex_t thread_lock;

    //start up async thread first
    rc = async_thread_init(&globals_r);
    if(rc != 0) {perror("async init failed!");return;}
    pthread_create(globals_r.async->pThread, 0, async_thread_start, globals_r.async);

    pThreadList = ngx_palloc( globals_r.pool, nThreads * sizeof(*pThreadList) );
    globals_r.threads = pThreadList;
	pthread_mutex_init(&thread_lock, NULL);
	 for( i = 0; i < nThreads; i++ )
    {
        int fds[2];
		threadInfo = ngx_palloc(globals_r.pool, sizeof(*threadInfo));
		if(!threadInfo) exit(STONE_ERROR_THREAD); //thread error
        // Store thread
        pThreadList[i] = threadInfo;

        // Thread stuff
        threadInfo->nThreadID = i;
        threadInfo->pThread = ngx_palloc(globals_r.pool, sizeof(pthread_t) );
        threadInfo->lock = &thread_lock;
        if(pipe(fds)){
            perror("pipe error");
            exit(STONE_ERROR_SYSCALL); //pipe error
        }
        threadInfo->receive_pipe = fds[0];
        threadInfo->send_pipe = fds[1];
        //INIT_LIST_HEAD(&threadInfo->con_que.head);
        //pthread_mutex_init(&threadInfo->con_que.lock, NULL);

        //
        // Here goes initialization of FastCGI
		//fcgx = ngx_palloc(globals_r.pool, sizeof(FCGX_Request));
        //FCGX_InitRequest( fcgx, 0, 0 );

        // Create the thread
        pthread_create( threadInfo->pThread, 0, worker_thread, (void *) threadInfo );
    }
    
    //while(1){sleep(1);}

    // accept here
    struct stone_conn *con;
    int rp_pid = 0;
    struct sockaddr sa;
    socklen_t salen = sizeof(sa);
    while( 1 )
    {
        //sleep( 1 );
        //malloc a new request here
        //con = opool_request(globals_r.connection);
        int result = accept(0, &sa, &salen);
        if (result <= 0){
            //opool_release(globals_r.connection, con);
            //free(con);
            continue;
        }
        /*
        con = malloc(sizeof(*con));
        memset(con, 0, sizeof(*con));
        FCGX_InitRequest ( &con->fcgx, 0, 0 );
        con->fcgx.ipcFd = result;
        con->fcgx.keepConnection = 1;
        result = FCGX_Accept_r( &con->fcgx );
        //push req into con_que
        pthread_mutex_lock(&threadInfo->con_que.lock);
        linux_list_add_tail(&con->head, &threadInfo->con_que.head);
        pthread_mutex_unlock(&threadInfo->con_que.lock);
        */
        rp_pid = rp_pid % nThreads;
        threadInfo = pThreadList[rp_pid];
        rp_pid ++;
        opool_release(threadInfo->con_que, (void *)result);
        if (write(threadInfo->send_pipe, "", 1) != 1){
            perror("pipe write error");
        }
    }
    //pthread_mutex_destroy( PTHREAD_MUTEX_INITIALIZER );
	app_close(0);
}

//error_handler = default_error_handler;
/*
 *here I try to use FCGX_APP
 FCGX_Init();
 FCGX_InitRequest( fcgx, 0, 0 );
 while (1){
	int result = FCGX_Accept_r( fcgx );
	FCGX_GetParam( "SCRIPT_NAME", fcgx->envp );
	FCGX_GetStr( contentData, req->env->ContentLength, fcgx->in );
	// headers and output content
	FCGX_PutStr( req->lpszOutput, nRetVal, req->fcgi_request->out );
	FCGX_Finish_r( fcgx );	
 }
 */
