#include 	"appsql.h"
#include    "notifier.h"
#include    "tree.h"
#include    "framework.h"
#include    "threadinfo.h"
//#include    "app.h"

void * app_select_store ( ngx_command_t *cmd, MYSQL_RES *result );
char * app_select_toString ( ngx_command_t *cmd, stone_node_t *tree );

unsigned int BKDRHash(char *str)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;
    while       (*str)
    {
        hash = hash * seed + (*str++);
    }
    return (hash & 0x7FFFFFFF);
}

int mysql_pool_init(void **res_ptr, opool_t *opool){
    MYSQL *mysql;
    mysql = mysql_init(NULL);
    *res_ptr = mysql;
    if(!mysql_real_connect(mysql, "localhost", "www", "www", "www", 0, "/run/mysqld/mysqld.sock",0)){
        return -1;
    }
    return 0;
}

int mysql_pool_destroy(void **res_ptr, opool_t *opool){
    if(*res_ptr)
        mysql_close(*res_ptr);
    return 0;
}

int db_mysql_init(notifier_block *nb, unsigned long ev, void *d){
    //MYSQL *mysql;
    struct GLOBAL_RAPHTERS *data;
    struct thread_info_t *threadInfo;
    opool_t *opool;
    if ( ev == NOTIFIER_INIT ){
        return NOTIFY_DONE;
        data = (struct GLOBAL_RAPHTERS*)d;
        /*
        mysql = mysql_init(NULL);
        printf("SQL ERROR!");
		return NOTIFIER_STOP;
        //exit(1);
        */
        opool = opool_create(data->pool, OPOOL_FLAG_LOCK, 4, mysql_pool_init, mysql_pool_destroy);
        if(!opool) return NOTIFY_STOP;
        //data->mysql = opool;
    }else if( ev == NOTIFIER_THREAD ){
        threadInfo = d;
        my_init();
        mysql_thread_init();
        threadInfo->mysql = ngx_palloc(threadInfo->pool, sizeof(MYSQL));
        if(threadInfo->mysql == NULL) return NOTIFY_STOP;
        if(!mysql_real_connect(threadInfo->mysql, "localhost", "www", "www", "www", 0, "/run/mysqld/mysqld.sock",0)){
            return NOTIFY_STOP;
        }
    }
    return NOTIFY_DONE;
}

int db_mysql_deinit(notifier_block *nb, unsigned long ev, void *d){
    struct GLOBAL_RAPHTERS *data;
    struct thread_info_t *threadInfo;
    if (ev == NOTIFIER_EXIT){
        return NOTIFY_DONE;
        data = (struct GLOBAL_RAPHTERS*)d;
        //if ( data->con ) mysql_close(data->con);
        //if(data->mysql){
        //    opool_destroy(data->mysql);
        //}
        //data->mysql = NULL;
    }else if(ev == NOTIFIER_THREAD_EXIT){
        mysql_thread_end();
    }
    return NOTIFY_DONE;
}

int module_mysql_init(void){
    int rc;
    rc = mysql_thread_safe();
    if(rc == 0){
        printf("mysql client is not pthread safe!!!");
        return -1;
    }
    //triger_set(NOTIFIER_INIT, 1, db_mysql_init);
    //triger_set(NOTIFIER_EXIT, 8, db_mysql_deinit);
    triger_set(NOTIFIER_THREAD, 1, db_mysql_init);
    triger_set(NOTIFIER_THREAD_EXIT, 8, db_mysql_deinit);
    return 0;
}

// @table table to select
// @fields select fields
// @where conditions
// @page @limit paginations
int app_select(stone_server_t *server, char * table, char * fields, struct list_head *wheres, int page, int limit, ngx_command_t *cmd){
	char *temp, *where, *query;
	stone_item_t *pos;
	stone_cache_command_t *policy;
	ngx_buf_t *buf;
    MYSQL *mysql;

	if (page <1){
        page = 1;
    }

    if (limit < 1){
        limit = 10;
    }

    int start = (page-1)*limit;
	temp = ngx_palloc(cmd->pool, 100);

	buf = ngx_create_temp_buf( cmd->pool, 200 );
	if ( wheres ) {
		list_for_each_entry( pos, wheres, ptr ){
			ngx_buf_cat ( cmd->pool, buf, " ", 1 );
			ngx_buf_cat ( cmd->pool, buf, pos->name, strlen( pos->name ) );
			ngx_buf_cat ( cmd->pool, buf, " = ", 3 );
			ngx_buf_cat ( cmd->pool, buf, pos->value, strlen( pos->value ) );
			ngx_buf_cat ( cmd->pool, buf, " AND", 4 );
		}
		if ( buf->last > buf->start ) *( buf->last - 4 ) = '\0';
		where = ngx_strdup ( cmd->pool, buf->start );
		ngx_buf_reset ( buf );
	}

	if ( query && *query ) {
		ngx_buf_cat ( cmd->pool, buf, "SELECT ", 7 );
		ngx_buf_cat ( cmd->pool, buf, fields, strlen( fields ) );
		ngx_buf_cat ( cmd->pool, buf, " FROM ", 6 );
		ngx_buf_cat ( cmd->pool, buf, table, strlen( table ) );
		if ( where && *where ) {
			ngx_buf_cat ( cmd->pool, buf, " WHERE ", 7 );
			ngx_buf_cat ( cmd->pool, buf, where, strlen( where ) );
		}
		sprintf ( temp, " LIMIT %d, %d", start, limit );
		ngx_buf_cat ( cmd->pool, buf, temp, strlen( temp ) );
	}else {
		return -1;
	}
	query = buf->start;
	policy = ( stone_cache_command_t * ) cmd->policy;
	if ( policy->read != NULL ){
		policy->read ( cmd, query );
	}

	if(cmd->data != NULL) return 0;
    //mysql = opool_request(globals_r.mysql);
    mysql = server->thread->mysql;
    if(!mysql) return -1;
    MYSQL_RES *result = app_query( mysql, query, cmd );
    //opool_release(globals_r.mysql, mysql);
	if ( result )
	{
		stone_node_t *tree;
		char *text;
		stone_node_command_t *tree_policy = &node_mjson_command;
		cmd->policy = tree_policy;
		
		tree = app_select_store ( cmd, result );
		mysql_free_result(result);
		//ngx_json_tree_to_string(cmd->pool, root, &text);
		text = app_select_toString( cmd, tree );
		//ngx_json_free_value(&root);
		//cmd->data = ngx_strdup(cmd->pool, text);
        cmd->data = text;

		// set policy to original
		cmd->policy = policy;
		if ( policy->write != NULL ){
			policy->write ( cmd, query );
		}
        return 0;
		//ngx_pfree(cmd->pool, text);
	}
    return -1;
}

// @query sql to exiecue
// @buf restult to output, to do
void * app_query(MYSQL *mysql, char * query, ngx_command_t *cmd){
    MYSQL_RES *result;
    MYSQL_ROW row;
	MYSQL_FIELD *fieldr;
	char *text;
    int t,flag,fields,num_rows;

    flag = mysql_real_query(mysql, query, strlen(query));
    if(flag){
        return 0;
    }else{
        result = mysql_store_result(mysql);
		return result;
	}
    //return num_rows;
}

void * app_select_store ( ngx_command_t *cmd, MYSQL_RES *result )
{
    MYSQL_ROW row;
	MYSQL_FIELD *fieldr;
// 	char *text;
    int t,fields,num_rows;
// 	stone_cache_command_t *policy;
// 	ngx_command_t tree_cmd;
	stone_node_command_t *tree_policy;
// 	tree_policy = &node_mjson_command;
	tree_policy = ( stone_node_command_t * )cmd->policy;

	fields = mysql_num_fields(result);
	fieldr = mysql_fetch_fields(result);
    num_rows = 0;
	// set policy of tree
 	cmd->policy = tree_policy;
	//json_t *root, *entry, *label, *value;
	stone_node_t *tree, *cur, *child;
	tree = tree_policy->create ( cmd, NULL, NODE_ARR );
		//root = ngx_json_new_array(cmd->pool);
        while(row = mysql_fetch_row(result)){
            num_rows++;
			// create an entry node
			//entry = ngx_json_new_object(cmd->pool);
			cur = tree_policy->create ( cmd, tree, NODE_OBJ );
            for(t=0;t<fields;t++){
                //response_write(res, row[t]);

				// insert the first label-value pair
// 				label = ngx_json_new_string(cmd->pool, fieldr[t].name);
// 				value = ngx_json_new_string(cmd->pool, row[t]);
// 				ngx_json_insert_child(label, value);
// 				ngx_json_insert_child(entry, label);
				child = tree_policy->create ( cmd, cur, NODE_VALUE );
				if ( !child ) return NULL;
				tree_policy->save ( cmd, child, fieldr[t].name, row[t] );
            }
            //ngx_json_insert_child(root, entry);
            //response_write(res, "\n");
        }
	return tree;
}

char * app_select_toString ( ngx_command_t *cmd, stone_node_t *tree )
{
	stone_node_command_t *tree_policy;
	char *text;

	tree_policy = ( stone_node_command_t * )cmd->policy;

	text = tree_policy->toString ( cmd, tree );
	return text;
}

unsigned int app_update(stone_server_t *server, char * table, struct list_head * fields, struct list_head * wheres, ngx_command_t *cmd)
{
	char *query, *where;
	stone_item_t *pos;
	ngx_buf_t *buf;
    MYSQL *mysql;

    mysql = server->thread->mysql;
    if (!mysql) return -1;
	
	buf = ngx_create_temp_buf( cmd->pool, 400 );

		list_for_each_entry( pos, fields, ptr ){
			ngx_buf_cat ( cmd->pool, buf, " ", 1 );
			ngx_buf_cat ( cmd->pool, buf, pos->name, strlen( pos->name ) );
			ngx_buf_cat ( cmd->pool, buf, " = ", 3 );
			ngx_buf_cat ( cmd->pool, buf, pos->value, strlen( pos->value ) );
			ngx_buf_cat ( cmd->pool, buf, ",", 1 );
		}
		if ( buf->last > buf->start ) *( buf->last - 1 ) = '\0';
		query = ngx_strdup ( cmd->pool, buf->start );
		ngx_buf_reset ( buf );

	if ( wheres ) {
		list_for_each_entry( pos, wheres, ptr ){
			ngx_buf_cat ( cmd->pool, buf, " ", 1 );
			ngx_buf_cat ( cmd->pool, buf, pos->name, strlen( pos->name ) );
			ngx_buf_cat ( cmd->pool, buf, " = ", 3 );
			ngx_buf_cat ( cmd->pool, buf, pos->value, strlen( pos->value ) );
			ngx_buf_cat ( cmd->pool, buf, " AND", 4 );
		}
		if ( buf->last > buf->start ) *( buf->last - 4 ) = '\0';
		where = ngx_strdup ( cmd->pool, buf->start );
		ngx_buf_reset ( buf );
	}

	if ( query && *query ) {
		ngx_buf_cat ( cmd->pool, buf, "UPDATE ", 7 );
		ngx_buf_cat ( cmd->pool, buf, table, strlen( table ) );
		ngx_buf_cat ( cmd->pool, buf, " SET ", 5 );
		ngx_buf_cat ( cmd->pool, buf, query, strlen( query ) );
		if ( where && *where ) {
			ngx_buf_cat ( cmd->pool, buf, " WHERE ", 7 );
			ngx_buf_cat ( cmd->pool, buf, where, strlen( where ) );
		}
	}else {
		return -1;
	}
// 	buf = ngx_create_temp_buf( cmd->pool, 400 );
// 	sprintf ( buf->start, sql, table, query, where );
	MYSQL_RES *result = app_query( mysql, buf->start, cmd );
	return 1;
}
