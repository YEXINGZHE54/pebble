#include "util.h"

void setcookie ( stone_server_t *server, char *name, char *value, int expires )
{
	char *cookie;
	cookie = ngx_palloc ( server->pool, strlen ( value ) + strlen ( name ) + 32 );
	sprintf ( cookie, "%s=%s; path=/;", name, value );

	HEADER ( "Set-Cookie", cookie );
    main_add_variable( server->pool, server->req->cookie, name, value );
}
