#include "framework.h"
#include "notifier.h"
#include "threadinfo.h"
#include <time.h>

#define nThreads 4
//static pthread_mutex_t PTHREAD_MUTEX_INITIALIZER;

void atquit(void){
	app_close(0);
}

void atquit_thread(void *d){
    triger(NOTIFIER_THREAD_EXIT, d);
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

void dispatch_thread(struct thread_info_t *threadInfo) {
    handler *cur;
	//stone_response_t *res;
	stone_server_t *server;
	ngx_pool_t *pool = threadInfo->pool;
	ngx_command_t command;
// 	unsigned int i;
	int method, rc;
    //getenv("PATH_INFO");
	char *path_info = FCGX_GetParam("PATH_INFO", threadInfo->fcgi_request->envp);
    if (path_info == NULL) {
        error_handler("NULL path_info", threadInfo->fcgi_request);
        return;
    }
   //getenv("REQUEST_METHOD");
	 char *method_str = FCGX_GetParam("REQUEST_METHOD", threadInfo->fcgi_request->envp);
    if (method_str == NULL) {
        error_handler("NULL method_str", threadInfo->fcgi_request);
        return;
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
        error_handler("unknown request method", threadInfo->fcgi_request);
        return;
    }

// 					command.pool = server->pool;
					command.policy = &redis_cache_cmd;
					command.resource = globals_r.redis;
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
					server = stone_app_start(threadInfo);
					command.pool = server->pool;
					store_path_info ( server, path_info, matches, cur->nmatch );
					/*rc = session_init ( server, &command );
					if ( rc != SESSION_OK ) {
						goto failure;
					}*/
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
		error_handler("error occured!", threadInfo->fcgi_request);
    success:
		//ngx_destroy_pool(pool);
		return;
}

void * worker_thread(void *data){
	struct thread_info_t *threadInfo = data;
	pthread_mutex_t *accept_mutex = threadInfo->lock;
	ngx_pool_t *pool;
	pool = ngx_create_pool(4096, globals_r.log);
	if (pool == NULL){
			error_handler("Error while creating pool buffer!", threadInfo->fcgi_request);
			return;
	}
	threadInfo->pool = pool;
    // add cleanup hook
    pthread_cleanup_push(atquit_thread, threadInfo);
    triger(NOTIFIER_THREAD, threadInfo);
	while(1){
		pthread_mutex_lock( accept_mutex );
        int result = FCGX_Accept_r( threadInfo->fcgi_request );
        pthread_mutex_unlock( accept_mutex );
        if(result != 0) continue;
		dispatch_thread(threadInfo);
		FCGX_Finish_r( threadInfo->fcgi_request );
		ngx_reset_pool(threadInfo->pool);
	}
    pthread_cleanup_pop(0);
}

void stone_run(void){
    int i;
	app_init();
	FCGX_Init();
	struct thread_info_t *threadInfo;
	struct thread_info_t **pThreadList;
    pthread_mutex_t thread_lock;
    pThreadList = ngx_palloc( globals_r.pool, nThreads * sizeof(*pThreadList) );
    globals_r.threads = pThreadList;
	pthread_mutex_init(&thread_lock, NULL);
	 for( i = 0; i < nThreads; i++ )
    {
		threadInfo = ngx_palloc(globals_r.pool, sizeof(*threadInfo));
		if(!threadInfo) exit(-1);
        // Store thread
        pThreadList[i] = threadInfo;

        // Thread stuff
        threadInfo->nThreadID = i;
        threadInfo->pThread = ngx_palloc(globals_r.pool, sizeof(pthread_t) );
        threadInfo->lock = &thread_lock;

        //
        // Here goes initialization of FastCGI
		threadInfo->fcgi_request = ngx_palloc(globals_r.pool, sizeof(FCGX_Request));
        FCGX_InitRequest( threadInfo->fcgi_request, 0, 0 );

        // Create the thread
        pthread_create( threadInfo->pThread, 0, worker_thread, (void *) threadInfo );
    }

    while( 1 )
    {
        sleep( 1 );
    }
    //pthread_mutex_destroy( PTHREAD_MUTEX_INITIALIZER );
	app_close(0);
}

//error_handler = default_error_handler;
/*
 *here I try to use FCGX_APP
 FCGX_Init();
 FCGX_InitRequest( threadInfo->fcgi_request, 0, 0 );
 while (1){
	int result = FCGX_Accept_r( threadInfo->fcgi_request );
	FCGX_GetParam( "SCRIPT_NAME", threadInfo->fcgi_request->envp );
	FCGX_GetStr( contentData, req->env->ContentLength, threadInfo->fcgi_request->in );
	// headers and output content
	FCGX_PutStr( req->lpszOutput, nRetVal, req->fcgi_request->out );
	FCGX_Finish_r( threadInfo->fcgi_request );	
 }
 */
