#include "session.h"
#include "nginx.h"
#include "list.h"
#include "cache.h"
#include "framework.h"
#include "command.h"
#include "tree.h"
#include "notifier.h"

struct list_head * empty_session ( ngx_pool_t * pool )
{
	struct list_head *session;
	session = ngx_palloc ( pool, sizeof ( struct list_head ) );
	if ( session ) INIT_LIST_HEAD ( session );
	return session;
}

/*
* we expect to store session in json in such format:
* { "uid" : 1234, "path" : "/" }
*/
int session_to_string ( ngx_command_t *cmd, struct list_head *session, char **t )
{
	uint rc;
	stone_node_t *root, *child;
	stone_item_t *pos;
    char *text;
    ngx_pool_t *pool = cmd->pool;
    stone_node_command_t *policy = cmd->policy;

	//root = json_new_object( pool );
    root = policy->create(cmd, NULL, NODE_OBJ);
	if ( root == NULL )
		return SESSION_ERROR;

	list_for_each_entry ( pos, session, ptr )
	{
		//json_insert_pair_into_object ( pool, root, pos->name, pos->value );
        child = policy->create(cmd, root, NODE_VALUE);
        policy->save(cmd, child, pos->name, pos->value);
	}

    text = policy->toString(cmd, root);
	//rc = json_tree_to_string ( pool, root, text );
	if ( text == NULL ) {
		return SESSION_ERROR;
	}
    *t = text;
	return SESSION_OK;
}

int string_to_session ( ngx_command_t *cmd, struct list_head **session, char *text )
{
	uint rc;
	//json_t **root, *child;
    stone_node_t *root, *child, *prev;
	stone_item_t *item;
    ngx_pool_t *pool;
    stone_node_command_t *policy;
    enum stone_node_type ntype;

    pool = cmd->pool;
    policy = cmd->policy;

    root = policy->fromString(cmd, text);
	//rc = json_parse_document (pool, root, text);
	if ( root == NULL ) {
		return SESSION_ERROR;
	}

	if (*session == NULL)
	{
		//create a empty session
		*session = empty_session( pool );
		if ( *session == NULL ) {
			return SESSION_ERROR	;
		}
	}

	rc = SESSION_OK;
	//child = ( *root )->child;
    prev = policy->get(cmd);
    policy->set(cmd, root);
    child = policy->child(cmd);
	while ( child )
	{
        //ntype = policy->type(child);
        /*
		if ( child->type != JSON_STRING )
			continue;
		if ( child->text == NULL || child->child->text == NULL )
			continue;
        */
		item = ngx_palloc ( pool, sizeof ( stone_item_t ) );
		if ( item == NULL )
		{
			rc = SESSION_ERROR;
			break;
		}
		//item->name = child->text;
        item->name = policy->name(child);
		//item->value = child->child->text;
        item->value = policy->value(child);
		linux_list_add_tail ( & ( item->ptr ), *session );
		//child = child->next;
        child = policy->next(cmd);
	}
    policy->set(cmd, prev);

	return rc;
}

char * session_rand ( stone_server_t *server, ngx_command_t *cmd )
{
	char *randstr, *text;
	int randtry, randnum;
	stone_cache_command_t *policy = ( stone_cache_command_t * ) cmd->policy;
	
	srand((unsigned)time(NULL));
	randtry = 0;
	randstr = ngx_palloc ( server->pool, 33 );
	// allow 10 tries
	while(randtry < 10){
		randtry++;
		randnum = rand();
		sprintf ( randstr, "%ld", randnum );
		text = policy->read ( cmd, randstr );
		if ( text != CACHE_ERROR )
			continue;
		else
			break;
	}

	if ( randtry >= 10 )
		goto failed;

	return randstr;
	
	failed:
		return NULL;
}

// init session
int session_init ( stone_server_t *server, ngx_command_t *cmd )
{
	int rc;
	char *text, *sid;
	stone_cache_command_t *policy = ( stone_cache_command_t * ) cmd->policy;
    
	server->session = empty_session ( server->pool );
    /*
	sid = _COOKIE ( SESSION_COOKIE );
	if ( !sid || !*sid ) {
		// create an empty session
        cmd->data = NULL;
		return SESSION_OK;
	}
    */
	sid = _COOKIE ( SESSION_COOKIE );
	if ( !sid || !*sid ) {
		sid = session_rand ( server, cmd );
		if ( sid == NULL ) return SESSION_ERROR;
		// add_header to init a session
		_SCOOKIE ( SESSION_COOKIE, sid );
        cmd->data = NULL;
        return SESSION_OK;
	}

	text = policy->read ( cmd, sid );
	if ( !text ) {
		return SESSION_ERROR;
	}
	return rc;
}

int session_destroy ( stone_server_t *server, ngx_command_t *cmd )
{
	char *text, *sid;
	stone_cache_command_t *policy = ( stone_cache_command_t * ) cmd->policy;
	
	sid = _COOKIE ( SESSION_COOKIE );

	if ( sid && *sid ) {
		// add header to delete cookie
		_DCOOKIE ( SESSION_COOKIE, sid );
		policy->delete ( cmd, sid );
	}

	if ( server->session ){
		server->session = NULL;
	}

	return SESSION_OK;
}

char * session_save ( ngx_command_t *cmd, char *sid )
{
	char *result; 
	stone_cache_command_t *policy = cmd->policy;

    result = policy->write(cmd, sid);
	return result;
}

int session_start(notifier_block *nb, unsigned long ev, void *d){
    stone_server_t *server = d;
    ngx_command_t *command;
    char *text;
    command = command_create(server->pool, COMMAND_CACHE_REDIS);
    command->resource = server->thread->redis;
    int rc = session_init(server, command);
    if(rc != SESSION_OK)
        return NOTIFY_STOP;	
	text = command->data;
    if(text == NULL) return NOTIFY_DONE; //empty_session
    command = command_create(server->pool, COMMAND_NODE_MJSON);
	rc = string_to_session ( command, &( server->session ), text );
    if(rc != SESSION_OK)
        return NOTIFY_STOP;
    return NOTIFY_DONE;
}

int session_stop(notifier_block *nb, unsigned long ev, void *d){
    stone_server_t *server = d;
    ngx_command_t *command;
    char *text, *result, *sid;
    int rc;
    command = command_create(server->pool, COMMAND_NODE_MJSON);
    command->resource = server->thread->redis;
    rc = session_to_string( command, server->session, &text);
    if(rc != SESSION_OK) return NOTIFY_STOP;
	command = command_create(server->pool, COMMAND_CACHE_REDIS);
    command->data = text;
    sid = _COOKIE( SESSION_COOKIE );
    if ( !sid || !*sid ) return NOTIFY_STOP;
    result = session_save ( command, sid);
	//result = policy->write ( command, sid );
	if ( result == NULL ) return NOTIFY_STOP;
    return NOTIFY_DONE;
}

int module_session_init(void){
    triger_set(NOTIFIER_START, 6, session_start);
    triger_set(NOTIFIER_STOP, 6, session_stop);
    return 0;
}
