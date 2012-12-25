#ifndef _STONE_SESSION_H
#define _STONE_SESSION_H

#include "nginx.h"
#include "list.h"
#include "app.h"
#include "command.h"

#define SESSION_OK 0
#define SESSION_ERROR -1
#define SESSION_COOKIE "stoneid"
/*
int session_to_json ( ngx_pool_t *pool, struct list_head *session, char **text );
int json_to_session ( ngx_pool_t *pool, struct list_head **session, char *text );
struct list_head * empty_session ( ngx_pool_t * pool );
int session_init ( stone_server_t *server, ngx_command_t *cmd );
int session_destroy ( stone_server_t *server, ngx_command_t *cd );
int session_save ( stone_server_t * server, ngx_command_t *cmd );
*/
#endif
