#ifndef STONE_TPL_H
#define STONE_TPL_H

#include "command.h"
#include "tree.h"
#include "list.h"

#define TPL_TAG_START   "{$ST:"
#define TPL_TAG_TAIL     "$ST}"
#define TPL_TAG_LOOP    "{$ST:loop"
#define TPL_TAG_VALUE   "{$ST:value"
#define TPL_TAG_IF   "{$ST:if"
#define TPL_TAG_INCLUDE   "{$ST:include"
#define TPL_TAG_FILE   "{$ST:file$ST}"
#define TPL_END_OF_TAG     "{$ST:end}"
#define TPL_KEY_BUF_SIZE 16
#define TPL_DIR "/opt/www/template/"
#define TPL_DATA_TABLE_SIZE 32
#define TPL_ASSIGN( name, value ) tpl_assign_tag( server->pool, server->tpl, name, value )
#define TPL_OUTPUT( name ) tpl_output( server, name )
#define tpl_hash hasht_hash
#define tpl_find_tag hasht_find
#define tpl_resize_data_table hasht_resize
#define tpl_assign_tag hasht_insert
//#define tpl_assign_tag( pool, tpl, name, value ) hasht_insert( pool, tpl, name, value )
#define tpl_unset_tag hasht_delete
#define tpl_update_tag hasht_update
//#define tpl_init_data_table( pool, size ) hasht_init( pool, size )
#define tpl_init_data_table hasht_init

struct tpl_loop_s {
	char *name;
	char *key;
	char *value;
};

#define tpl_data_table hasht_table
#define tpl_data_node  hasht_node

#include "app.h"

//int tpl_init ( void );
tpl_data_table *hasht_init ( ngx_pool_t *pool, uint mask );
stone_node_t *parse_tpl ( ngx_command_t *cmd, char **str );
//char * tpl_render ( ngx_command_t *cmd, ngx_command_t *dcmd, ngx_buf_t *buf, tpl_data_table * );
char * tpl_load ( ngx_command_t *cmd, char *fname );
//char *tpl_output ( ngx_pool_t *pool, tpl_data_table *table, char *name );
char *tpl_output ( stone_server_t *server, char *name );
#endif
