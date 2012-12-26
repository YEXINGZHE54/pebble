#include "app.h"
#include "hiredis/hiredis.h"
#include "tpl.h"
#include "notifier.h"
#include "module.h"
#include <signal.h>
#include <mysql/mysql.h>

int module_global_init(void){
    ngx_pool_t *pool;
	ngx_log_t *log;
    pool = ngx_create_pool(4096, log);
    if (pool == NULL){
		app_close(1);
        printf("Error while creating pool buffer!");
        return -1;
    }
    globals_r.pool = pool;
    globals_r.log  = log;
    return 0;
}
/*
int module_global_init(void){
    triger_set(NOTIFIER_INIT, 0, global_init);
    return 0;
}*/

unsigned int app_init(){
	signal(SIGHUP,  app_close);
    signal(SIGUSR1, app_close);
    signal(SIGTERM, app_close);
    signal(SIGCHLD, app_close);

    notifier_chain_init();
    module_init();
    int ret = triger( NOTIFIER_INIT, &globals_r );
    if((ret & NOTIFIER_STOP) == NOTIFIER_STOP) exit(-1);

//	init_handlers(globals_r.pool);

}

void app_close(int num){
    triger ( NOTIFIER_EXIT, &globals_r);
    //extern struct GLOBAL_RAPHTERS globals_r;
	//cleanup_handlers(globals_r.pool);
    // should free the total memory
    if ( globals_r.pool ) ngx_destroy_pool(globals_r.pool);
//	globals_r.pool = NULL;
//	globals_r.con = NULL;
}


stone_server_t * stone_app_start(struct thread_info_t *threadInfo ){
	ngx_pool_t *pool;
	stone_server_t *server;
	stone_request_t *req;
	stone_response_t *res;
	tpl_data_table *tpl_data;

	int rc;
	//create server struct
	server = ngx_palloc(threadInfo->pool, sizeof(stone_server_t));
	if (server == NULL){
			error_handler("Error while initializing server info!", threadInfo->fcgi_request);
			return NULL;
	}
	
	//create pool first
	pool = ngx_create_pool(4096, globals_r.log);
	if (pool == NULL){
			error_handler("Error while creating pool buffer!", threadInfo->fcgi_request);
			return NULL;
	}

	server->pool = pool;
	server->thread = threadInfo;
    server->fcgx = threadInfo->fcgi_request;
    triger ( NOTIFIER_START, server );
	// create request and response
	return server;
}

int stone_app_stop(stone_server_t *server){
    triger ( NOTIFIER_STOP, server );
	ngx_destroy_pool(server->pool);
	return 0;
}
