#ifndef _COMMAND_H
#define _COMMAND_H
#include "nginx.h"
typedef struct ngx_command_s     ngx_command_t;
typedef struct stone_cache_command_s stone_cache_command_t;

enum ngx_command_type { COMMAND_CACHE_FILE = 0, COMMAND_CACHE_REDIS = 1, COMMAND_CACHE_DB = 2, COMMAND_NODE_MJSON = 3 };

struct ngx_command_s {
    ngx_pool_t		*pool;
	// needed resource for command funcs
	void *resource;
    enum ngx_command_type	type;
	//struct stone_cache_command_s	*policy;
	void *policy;
	// to store data needed or generated in policy command
    void            *data;
};

struct stone_cache_command_s {
	char           	*(*read)(ngx_command_t *cmd, void *arg);
	char           	*(*write)(ngx_command_t *cmd, void *arg);
	char           	*(*delete)(ngx_command_t *cmd, void *arg);
};

ngx_command_t * command_create ( ngx_pool_t *, enum ngx_command_type );
ngx_command_t* command_clone ( ngx_pool_t *, ngx_command_t * );

#endif