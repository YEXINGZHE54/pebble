/*
    Copyright (C) 2011 Raphters authors,
    
    This file is part of Raphters.
    
    Raphters is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Raphters is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "framework.h"
#include "simple.h"
#include "cache.h"

extern void stone_run(void);
void simple_func(stone_server_t *);

/* START_HANDLER (simple, GET, "simple/\\(.*\\)", res, 2, matches) { */
void simple_func(stone_server_t *server){
	ngx_pool_t *pool;
    char *query, buf[200] = {'\0'}, *dest;
    int flag,t,len;
    app_sql_value query_value;
	ngx_command_t *cache_command;

	pool = server->pool;
    HEADER ( "content-type", "text/html" );
    //response_write(res, "hello world<br/>");
    //response_write(res, "wonder world!<br>");
	/*
    char *path = getenv("PATH_INFO");
    regmatch_t preg = matches[1];
    len = preg.rm_eo - preg.rm_so;
    //memset(dest, 0, 20);
    strncpy(dest, path+preg.rm_so, len);
    */
	dest = server->req->path[1];
    //response_write(res, dest);

//     query = "id = ";
    //query = ngx_palloc(pool, 6);
    //strncpy(query, "id = ", 6);
//     strncpy(buf, query, strlen(query));
//     strncpy(buf+strlen(query), dest, len);
	LIST_HEAD( wheres );
	main_add_variable ( pool, &wheres, "id", dest );
	LIST_HEAD( fields );
	main_add_variable ( pool, &fields, "category_id", ngx_strdup( pool, "211" ) );
	//sprintf(buf, "id = %d", *dest);
	//cache_command = {pool,	0,	cache_read,	cache_write,	NULL};
	//cache_command.run  = cache_read;
	//cache_command.type = 0;
	//cache_command.policy = &file_cache_cmd;
	//cache_command.post = cache_write;
	//cache_command.pool = pool;
    //cache_command.data = NULL;
    //response_write(res, buf);
    cache_command = command_create(pool, COMMAND_CACHE_REDIS);
    cache_command->resource = server->thread->redis;
    int rc = app_select(server, "article", "*", &wheres, 1, 0, cache_command);
    if (rc != 0) ECHO("error data");
	//app_update(globals_r.con, "article", &fields, &wheres, &cache_command);
	//ECHO(buf);
    //if (cache_command.data) response_write(pool, res, cache_command.data);
    if (cache_command->data) ECHO(cache_command->data);
    /*
	cache_command.policy = &node_mjson_command;
	char *str = tpl_load( &cache_command, "index.tpl");
	if ( !str ) return;
	stone_node_t *node;
	node = parse_tpl ( &cache_command, &str );
	if ( !node ) return;
	cache_command.data = node;
	ngx_command_t *dcommand = command_clone( cache_command.pool, &cache_command );
	ngx_buf_t *tplbuf = ngx_create_temp_buf( cache_command.pool, 4096 );
	if ( !tplbuf ) return;
	str = tpl_render( &cache_command, dcommand, tplbuf, server->tpl);
    */
    char *val = _GET( "act" );
    //TPL_ASSIGN ( ngx_strdup ( pool, "act" ), val );
    char *str = TPL_OUTPUT ( "index.tpl" );
	if ( str ) ECHO ( str );
	
	char *temp = _GET ( "act" );
	if (temp != NULL )
		ECHO ( temp );
}

/*
START_HANDLER (default_handler, GET, "", res, 0, matches) {
    response_add_header(res, "content-type", "text/html");
    response_write(res, "default page");
} END_HANDLER
*/
int main() {
    void *data;
    void (*proc)(void *);
    extern void worker_process(void *);
    int n,i;

	handler h = {simple_func, GET, "simple/\\([0-9]*$\\)", {0}, 2};
	add_handler(&h);
	/*
    app_init();
	
//     add_handler(simple);
//     add_handler(default_handler);
    data = &globals_r;
    proc = &worker_process;
    n = 1;
    for(i=0;i<n;i++){
        spawn_worker(data, proc);
    }

    (*proc)(data);
	
	app_close(0);
	*/
	stone_run();
    return 0;
}

