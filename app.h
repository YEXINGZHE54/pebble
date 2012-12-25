#ifndef STONE_APP
#define STONE_APP

typedef struct stone_server_s stone_server_t;

#include <mysql/mysql.h>
#include "hiredis/hiredis.h"
#include "list.h"
#include "request.h"
#include "response.h"
#include "tpl.h"
#include "threadinfo.h"
#include "opool.h"

#define STONE_PAGE_SIZE_MAX 4096
#define STONE_UPLOAD_LEN_MAX 20*1024*1004
#define STONE_BOUNDARY_LEN_MAX 128
#define TMP_DIR "/tmp/stone/"


typedef struct
{
    int threads;
    const char* name;
    const char* pwd;
} configuration;

struct stone_item_s{
	char *name;
	void *value;
	struct list_head ptr;
};
typedef struct stone_item_s stone_item_t;

struct stone_server_s {
	stone_request_t *req;
	ngx_pool_t *pool;
	stone_response_t *res;
    tpl_data_table *tpl;
	struct list_head *session;
	struct thread_info_t *thread;
};

struct GLOBAL_RAPHTERS {
    configuration *config;
    opool_t *mysql;
	opool_t *redis;
    ngx_pool_t *pool;
    hasht_table *tpl;
	ngx_log_t *log;
    struct thread_info_t **threads;
}globals_r;

unsigned int app_init(void);
void app_close(int);
stone_server_t * stone_app_start(struct thread_info_t *);
int stone_app_stop(stone_server_t *);

#endif
