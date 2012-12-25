#ifndef _CACHE_H
#define _CACHE_H
#include "command.h"
#include "nginx.h"

#define app_cache_dir "/opt/www/upload/"
#define CACHE_ERROR NULL

char	*cache_read(ngx_command_t *cmd, void *arg);
char	*cache_write(ngx_command_t *cmd, void *arg);
char	*cache_delete(ngx_command_t *cmd, void *arg);
char	*redis_read(ngx_command_t *cmd, void *arg);
char	*redis_write(ngx_command_t *cmd, void *arg);
char	*redis_delete(ngx_command_t *cmd, void *arg);
char	*db_read(ngx_command_t *cmd, void *arg);
char	*db_write(ngx_command_t *cmd, void *arg);
char	*db_delete(ngx_command_t *cmd, void *arg);

static struct stone_cache_command_s file_cache_cmd = { .read = cache_read, .write = cache_write, .delete = cache_delete };
static struct stone_cache_command_s redis_cache_cmd = { .read = redis_read, .write = redis_write, .delete = redis_delete };
static struct stone_cache_command_s db_cache_cmd = { .read = db_read, .write = db_write, .delete = db_delete };

#endif