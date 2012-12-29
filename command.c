#include "command.h"
#include "cache.h"
#include "tree.h"
#include "app.h"

ngx_command_t * command_create ( ngx_pool_t *pool, enum ngx_command_type type )
{
	ngx_command_t *cmd;
	cmd = ngx_palloc ( pool, sizeof ( *cmd ) );
	if ( !cmd ) return NULL;
	cmd->pool = pool;
	cmd->type = type;
	switch ( type )
	{
		case COMMAND_CACHE_FILE:
			cmd->policy = &file_cache_cmd;
			cmd->resource = app_cache_dir;
			break;
		case COMMAND_CACHE_REDIS:
			cmd->policy = &redis_cache_cmd;
			//cmd->resource = globals_r.redis;
			break;
		case COMMAND_CACHE_DB:
			cmd->policy = &db_cache_cmd;
			//cmd->resource = globals_r.mysql;
			break;
		case COMMAND_NODE_MJSON:
			cmd->policy = &node_mjson_command;
			break;
		default:
			return NULL;
	}
	return cmd;
}

ngx_command_t* command_clone ( ngx_pool_t *pool, ngx_command_t *origin )
{
	ngx_command_t *cmd;
	cmd = ngx_palloc ( pool, sizeof ( *cmd ) );
	if ( !cmd ) return NULL;
	cmd->policy = origin->policy;
	cmd->pool = origin->pool;
	cmd->data = origin->data;
	cmd->resource = origin->resource;
	cmd->type = origin->type;
	return cmd;
}
