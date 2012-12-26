#include "tpl.h"
#include "notifier.h"
#include "request.h"
#include "nginx.h"
#include "app.h"
#include <assert.h>
#include <pthread.h>

static pthread_mutex_t global_tpl_lock;

int tpl_init (notifier_block *nb, unsigned long ev, void *d)
{
    struct GLOBAL_RAPHTERS *data;
    data = (struct GLOBAL_RAPHTERS*)d;
    hasht_table *table;
    table = hasht_init ( data->pool, TPL_DATA_TABLE_SIZE );
    if ( !table ) return NOTIFY_STOP;
    data->tpl = table;
    return NOTIFY_DONE;
}

int tpl_start (notifier_block *nb, unsigned long ev, void *d){
    stone_server_t *server = d;
    tpl_data_table *tpl_data;
    tpl_data = tpl_init_data_table(server->pool, TPL_DATA_TABLE_SIZE);
    if(!tpl_data)
        return NOTIFY_STOP;
    server->tpl = tpl_data;
    return NOTIFY_DONE;
}

int module_tpl_init(void){
    pthread_mutex_init(&global_tpl_lock, NULL);
    triger_set(NOTIFIER_INIT, 5, tpl_init);
    triger_set(NOTIFIER_START, 9, tpl_start);
    return 0;
}

stone_node_t *parse_tpl ( ngx_command_t *cmd, char **str )
{
	assert ( str );
	assert ( *str );
	char *buf = *str;
	char *name_buf, *key_buf, *value_buf, *tag_tail, *tag_end, *buf_start, *name, *value, *key;
	uint state, eof = 1;
	int test;
	struct tpl_loop_s *loop_struc;
	stone_node_t *root, *cur, *child;
	stone_node_command_t *policy = ( stone_node_command_t * ) cmd->policy;
	// buf = loop || buf = if || buf = value
	parse_tag_context:
		// loop
		if ( strncmp ( buf, TPL_TAG_LOOP, strlen ( TPL_TAG_LOOP ) ) == 0 ) goto loop_context;
		if ( strncmp ( buf, TPL_TAG_IF, strlen ( TPL_TAG_IF ) ) == 0 ) goto if_context;
		if ( strncmp ( buf, TPL_TAG_VALUE, strlen ( TPL_TAG_VALUE ) ) == 0 ) goto value_context;
        if ( strncmp ( buf, TPL_TAG_INCLUDE, strlen ( TPL_TAG_INCLUDE ) ) == 0 ) goto include_context;
        if ( strncmp ( buf, TPL_TAG_FILE, strlen ( TPL_TAG_FILE ) ) == 0 ) goto file_context;
        return NULL;
    file_context:
		buf += strlen (  TPL_TAG_FILE ); //skip loop
		// create root node, default is obj
		root = policy->create( cmd, NULL, NODE_OBJ );
		if ( !root ) return NULL;
		cur = root;
		goto parse_body;
		return NULL;
    include_context:
		buf += strlen (  TPL_TAG_INCLUDE ); //skip loop
		tag_tail = strstr ( buf, TPL_TAG_TAIL ); // find tag_tail
		if ( !tag_tail ) return NULL; // no tag_tail, means error!
		value_buf = ngx_palloc ( cmd->pool, TPL_KEY_BUF_SIZE );
		if ( !value_buf ) return NULL;
		name = value_buf;
		while ( *buf && ( buf < tag_tail ) )
		{
			if ( *buf == ' ' )
			{
				buf++;
				continue;
			}
			*name++ = *buf++;
		}
		if ( !*buf ) return NULL;
		eof = 0; // it does not need end of tag
		name = value_buf;
        char *tpl_string = tpl_load ( cmd, name );
        if ( !tpl_string ) return NULL;
        root = parse_tpl ( cmd, &tpl_string );
		if ( !root ) return NULL;
        cur = root;
		buf += strlen ( TPL_TAG_TAIL );
		goto parse_tag_end;
	value_context:
		buf += strlen (  TPL_TAG_VALUE ); //skip loop
		tag_tail = strstr ( buf, TPL_TAG_TAIL ); // find tag_tail
		if ( !tag_tail ) return NULL; // no tag_tail, means error!
		value_buf = ngx_palloc ( cmd->pool, TPL_KEY_BUF_SIZE );
		name_buf = ngx_palloc ( cmd->pool, TPL_KEY_BUF_SIZE );
		if ( !value_buf || !name_buf ) return NULL;
		name = name_buf;
		while ( *buf && ( buf < tag_tail ) )
		{
			if ( *buf == ' ' )
			{
				buf++;
				continue;
			}
			if ( *buf == '.' )
			{
				*name = '\0';
				name = value_buf;
				buf++;
				continue;
			}
			*name++ = *buf++;
		}
		if ( !*buf ) return NULL;
		eof = 0; // it does not need end of tag
		root = policy->create( cmd, NULL, NODE_VALUE );
		if ( !root ) return NULL;
		cur = root;
// 		name = (*value_buf) ? name_buf : NULL;
		name = name_buf;
		value = (*value_buf) ? value_buf : name_buf;
		policy->save ( cmd, cur, name, value );
		buf += strlen ( TPL_TAG_TAIL );
		goto parse_tag_end;
	if_context:
		buf += strlen (  TPL_TAG_IF ); //skip loop
		tag_tail = strstr ( buf, TPL_TAG_TAIL ); // find tag_tail
// 		tag_end = strstr ( buf, TPL_END_OF_TAG ); // find end of tag
		if ( !tag_tail /*|| !tag_end || tag_tail >= tag_end*/ ) return NULL; // no tag_tail, means error!
		value_buf = ngx_palloc ( cmd->pool, TPL_KEY_BUF_SIZE );
		name_buf = ngx_palloc ( cmd->pool, TPL_KEY_BUF_SIZE );
		if ( !value_buf || !name_buf ) return NULL;
		name = name_buf;
		while ( *buf && ( buf < tag_tail ) )
		{
			if ( *buf == ' ' )
			{
				buf++;
				continue;
			}
			if ( *buf == '.' )
			{
				*name = '\0';
				name = value_buf;
				buf++;
				continue;
			}
			*name++ = *buf++;
		}
		if ( !*buf ) return NULL;
		root = policy->create( cmd, NULL, NODE_CONDITION );
		if ( !root ) return NULL;
		cur = root;
// 		name = (*value_buf) ? name_buf : NULL;
		name = name_buf;
		value = (*value_buf) ? value_buf : name_buf;
		policy->save ( cmd, cur, name, value );
		buf += strlen ( TPL_TAG_TAIL );
		goto parse_body;
	loop_context:
		buf += strlen (  TPL_TAG_LOOP ); //skip loop
		tag_tail = strstr ( buf, TPL_TAG_TAIL ); // find tag_tail
// 		tag_end = strstr ( buf, TPL_END_OF_TAG ); // find end of tag
		if ( !tag_tail /*|| !tag_end || tag_tail >= tag_end*/ ) return NULL; // no tag_tail, means error!
		state = 0;
		name = name_buf = ngx_palloc ( cmd->pool, TPL_KEY_BUF_SIZE );
		key = key_buf = ngx_palloc ( cmd->pool, TPL_KEY_BUF_SIZE );
		value = value_buf = ngx_palloc ( cmd->pool, TPL_KEY_BUF_SIZE );
		if ( !name || !key || !value ) return NULL;
		while ( *buf && ( buf < tag_tail ) )
		{
			if ( *buf == ' ' )
			{
				buf++;
				continue;
			}
			if ( *buf == '=' && state == 0 )
			{
				state = 1;
				buf++;
				continue;
			}
			if ( *buf == '=' && *(buf+1) == '>' && state == 1 )
			{
				state = 2;
				buf += 2;
				continue;
			}
			switch ( state )
			{
				case 0:
					*name++ = *buf;
					break;
				case 1:
					*key++ = *buf;
					break;
				case 2:
					*value++ = *buf;
					break;
				default:
					return NULL;
			}
			buf++;
		}
		if ( !*buf ) return NULL;
		// here comes to an end
		loop_struc = ngx_palloc( cmd->pool, sizeof ( *loop_struc ) );
		loop_struc->name = name_buf;
		loop_struc->key = ( state == 2 ) ? key_buf : ngx_strdup( cmd->pool, "k" );
		loop_struc->value = ( state == 2 ) ? value_buf : key_buf;
		root = policy->create( cmd, NULL, NODE_ARR );
		if ( !root ) return NULL;
		cur = root;
// 		cur->value = loop_struc;
// 		cur->name = name_buf;
		policy->save ( cmd, cur, name_buf, loop_struc );
		buf += strlen ( TPL_TAG_TAIL );
		goto parse_body;
	parse_body:
		buf_start = buf;
		if ( eof )
		{
			tag_end = strstr ( buf, TPL_END_OF_TAG );
			if ( !tag_end ) return NULL;
		}

		while ( *buf )
		{
			if ( eof && ( buf >= tag_end ) ) break;
			// means a tag start, we should collect const chars and start another parsing
			if ( strncmp ( buf, TPL_TAG_START, strlen ( TPL_TAG_START ) ) == 0 )
			{
				if ( buf > buf_start )
				{
					child = policy->create ( cmd, cur, NODE_NC );
					if ( !child ) return NULL;
					value = ngx_palloc ( cmd->pool, buf - buf_start + 1 );
					if ( !value ) return NULL;
					strncpy ( value, buf_start, buf-buf_start );
					policy->save ( cmd, child, NULL, value );
					value = NULL;
				}
				child = parse_tpl ( cmd, &buf );
				if ( !child ) return NULL;
// 				cmd->data = child;
// 				policy->set ( cmd, child );
				policy->append ( cur, child );
// 				policy->parent ( cmd );
				buf_start = buf;
				if ( eof )
				{
					tag_end = strstr ( buf, TPL_END_OF_TAG );
					if ( !tag_end ) return NULL;
				}
				continue;
			}
			buf++;
			continue;
		}
		if ( eof && ( !*buf ) ) return NULL;
		// here comes to tag_end
		if ( buf > buf_start )
		{
			child = policy->create ( cmd, cur, NODE_NC );
			if ( !child ) return NULL;
			value = ngx_palloc ( cmd->pool, buf - buf_start + 1 );
			if ( !value ) return NULL;
			strncpy ( value, buf_start, buf-buf_start );
			policy->save ( cmd, child, NULL, value );
			value = NULL;
// 			policy->parent ( cmd );
		}
		buf_start = buf;
	parse_tag_end:
		if ( eof )
			buf += strlen ( TPL_END_OF_TAG );
		*str = buf;
		return root;
}

// cmd, contains tpl node tree and dcmd contains data
char * tpl_render ( ngx_command_t *cmd, ngx_command_t *dcmd, ngx_buf_t *buf, tpl_data_table *data )
{
    stone_node_t *cur, *loop_data, *tpl_node, *tpl_data, *tpl;
    stone_node_command_t *policy, *dpolicy;
	char *name, *key, *value, *index_str, *temp, pbuf[TPL_KEY_BUF_SIZE];
	void *dcmd_data;
	int index;
	enum stone_node_type node_type;
    policy = ( stone_node_command_t * )cmd->policy;
	tpl = (stone_node_t *)policy->get ( cmd );
// 	dcmd = command_clone( cmd->pool, cmd );
	dpolicy = ( stone_node_command_t * )dcmd->policy;
    cur = tpl;
	node_type = policy->type( cur );
	if ( node_type == NODE_NC )
	{
// 		ngx_buf_cat ( cmd->pool, buf, policy->value ( cur ), strlen ( policy->value ( cur ) ) );
		temp = policy->value ( cur );
		ngx_buf_cat ( cmd->pool, buf, temp, strlen ( temp ) );
		return buf->start;
	}
    else if ( node_type == NODE_ARR ) 
	{
		name = ( char * )policy->name ( cur );
		struct tpl_loop_s *loop_struc = (struct tpl_loop_s *) policy->value ( cur );
		if ( !name || !loop_struc ) return NULL; // tpl error
		key = loop_struc->key;
		value = loop_struc->value;
		loop_data = tpl_find_tag ( data, name );
		if ( !loop_data ) return buf->start; // params undefined, not error
		dcmd_data = dpolicy->get ( dcmd );
		dpolicy->set ( dcmd, loop_data );
		tpl_data = dpolicy->child ( dcmd );
		index = 0;
		while ( tpl_data )
		{
			ngx_memzero ( pbuf, sizeof ( pbuf ) );
			sprintf( pbuf, "%d", index );
			index_str = ngx_strdup( cmd->pool, pbuf );
			if ( tpl_update_tag( cmd->pool, data, value, tpl_data ) != 0 )
                return NULL;
			if ( tpl_update_tag( cmd->pool, data, key, index_str ) != 0 )
                return NULL;
			tpl_node = policy->child ( cmd );
			while ( tpl_node )
			{
				tpl_render ( cmd, dcmd, buf, data );
				tpl_node = policy->next ( cmd );
			}
			policy->parent ( cmd );
			tpl_data = dpolicy->next ( dcmd );
			index ++;
		}
		dpolicy->set ( dcmd, dcmd_data );
		return buf->start;
	}
	else if ( node_type == NODE_VALUE )
	{
		name = ( char * )policy->name ( cur );
		if ( !name ) return buf->start;
		loop_data = tpl_find_tag ( data, name );
		if ( !loop_data ) return buf->start;
		value = (char *)policy->value ( cur );
		//tpl_data = ( value && ( name != value ) ) ? policy->search ( loop_data, value ) : loop_data;
		if ( value && ( name != value ) ) 
        {
            tpl_data = policy->search ( loop_data, value );
		    if ( !tpl_data ) return buf->start;
		    temp = dpolicy->value ( tpl_data );
        }
        else
        {
            temp = ( char * ) loop_data;
        }
		if ( !temp ) return buf->start;
		ngx_buf_cat ( cmd->pool, buf, temp, strlen ( temp ) );
		return buf->start;
	}
	else if ( node_type == NODE_CONDITION )
	{
		name = ( char * )policy->name ( cur );
		if ( !name ) return buf->start;
		// how to get data ? It is a question
		loop_data = tpl_find_tag ( data, name );
		if ( !loop_data ) return buf->start;
		value = (char *)policy->value ( cur );
		//tpl_data = ( value && ( name != value ) ) ? policy->search ( loop_data, value ) : loop_data;
		//if ( !tpl_data ) return buf->start;
		//temp = dpolicy->value ( tpl_data );
		if ( value && ( name != value ) ) 
        {
            tpl_data = policy->search ( loop_data, value );
		    if ( !tpl_data ) return buf->start;
		    temp = dpolicy->value ( tpl_data );
        }
        else
        {
            temp = ( char * ) loop_data;
        }
		if ( !temp ) return buf->start;
		tpl_node = policy->child ( cmd );
		while ( tpl_node )
		{
			tpl_render ( cmd, dcmd, buf, data );
			tpl_node = policy->next ( cmd );
		}
		policy->parent ( cmd );
		return buf->start;
	}
    else if ( node_type == NODE_OBJ )
	{
		tpl_node = policy->child ( cmd );
		while ( tpl_node )
		{
			tpl_render ( cmd, dcmd, buf, data );
			tpl_node = policy->next ( cmd );
		}
		policy->parent ( cmd );
		return buf->start;
	}
	else
		return NULL;
}

char * tpl_load ( ngx_command_t *cmd, char *fname )
{
	char *path = TPL_DIR;
	char             *buf,*from, *data;
    off_t             size;
    size_t            len;
    ssize_t           n;
    ngx_fd_t          fd;
    ngx_file_info_t   fi;

    data = buf = NULL;
	//cache_file = (char*)ngx_palloc(cmd->pool, 1000);
	//ngx_memzero(cache_file, 1000);
	//sprintf(cache_file, "%s", fhash);
	//strncpy(cache_file, "id1234", 6);
	from = ngx_palloc(cmd->pool, sizeof ( TPL_DIR ) + 1 + strlen ( fname ) );
	sprintf(from, "%s%s", path, fname);

    fd = ngx_open_file(from, NGX_FILE_RDONLY, NGX_FILE_OPEN, NGX_FILE_DEFAULT_ACCESS);

    if (fd == NGX_INVALID_FILE) {
        //ngx_log_error(NGX_LOG_CRIT, cf->log, ngx_errno, ngx_open_file_n " \"%s\" failed", from);
        goto failed;
    }

        if (ngx_fd_info(fd, &fi) == NGX_FILE_ERROR) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_fd_info_n " \"%s\" failed", from);

            goto failed;
        }

        size = ngx_file_size(&fi);


    len = 65536;

    if ((off_t) len > size) {
        len = (size_t) size;
    }

    buf = ngx_palloc(cmd->pool, len+1+strlen( TPL_END_OF_TAG ) + strlen( TPL_TAG_FILE ) );
    if (buf == NULL) {
        goto failed;
    }
    data = buf;

    strncpy ( buf, TPL_TAG_FILE, strlen ( TPL_TAG_FILE ) );
    buf += strlen ( TPL_TAG_FILE );
    while (size > 0) {

        if ((off_t) len > size) {
            len = (size_t) size;
        }

        n = ngx_read_fd(fd, buf, len);

        if (n == NGX_FILE_ERROR) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_read_fd_n " \"%s\" failed", from);
            goto failed;
        }

        if ((size_t) n != len) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_read_fd_n " has read only %z of %uz from %s", n, size, from);
            goto failed;
        }
        buf += n;
        size -= n;
    }
    strncpy ( buf, TPL_END_OF_TAG, strlen ( TPL_TAG_FILE ) );
    buf += strlen ( TPL_END_OF_TAG );

success:

    if (fd != NGX_INVALID_FILE) {
        if (ngx_close_file(fd) == NGX_FILE_ERROR) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_close_file_n " \"%s\" failed", from);
        }
    }

    return data;
failed:
    if (fd != NGX_INVALID_FILE) {
        if (ngx_close_file(fd) == NGX_FILE_ERROR) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_close_file_n " \"%s\" failed", from);
        }
    }
	return NULL;
}

char *tpl_output ( stone_server_t *server, char *name )
{
    ngx_command_t *cache_command, *dcommand;
	stone_node_t *node;
	ngx_buf_t *tplbuf;
    char *str;
    ngx_pool_t *pool = server->pool;
    tpl_data_table *table = server->tpl;
    triger(NOTIFIER_TPLOUT, server);
    cache_command = command_create ( pool, COMMAND_NODE_MJSON );
    node = hasht_find ( globals_r.tpl, name );
    if ( !node )
    { 
        pthread_mutex_lock(&global_tpl_lock);
        cache_command->pool = globals_r.pool;
	    str = tpl_load( cache_command, name );
	    if ( !str ) goto release_tpl_lock;
	    node = parse_tpl ( cache_command, &str );
	    if ( !node ) goto release_tpl_lock;
        hasht_insert ( globals_r.pool, globals_r.tpl, name, node );
        goto success;
release_tpl_lock:
        pthread_mutex_unlock(&global_tpl_lock);
        return NULL;
success:
        pthread_mutex_unlock(&global_tpl_lock);
        cache_command->pool = pool;
    }
tpl_found:
	cache_command->data = node;
	dcommand = command_clone( pool, cache_command );
	tplbuf = ngx_create_temp_buf( pool, 4096 );
	if ( !tplbuf ) return NULL;
	str = tpl_render( cache_command, dcommand, tplbuf, table);
    return str;
}
