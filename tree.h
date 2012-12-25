#ifndef _STONE_TREE_H
#define _STONE_TREE_H

#include "list.h"
#include "command.h"

// nc means const string, default name is NC, value means key and value, arr means arrays of elements, obj means object
enum stone_node_type { NODE_NC = 0, NODE_VALUE = 1, NODE_ARR = 2, NODE_OBJ = 3, NODE_CONDITION = 4 };

typedef struct stone_node_s stone_node_t;
typedef struct stone_node_command_s stone_node_command_t;

struct stone_node_s {
	char * name;
	void * value;
	stone_node_t *next;
    stone_node_t *child;
    stone_node_t *parent;
	enum stone_node_type type;
};

// status machine in node tree
struct stone_node_command_s {
    // create a node ( child of root if root not null ) and save it to cmd->data
	stone_node_t * ( *create ) ( ngx_command_t *cmd, stone_node_t * root, enum stone_node_type type );
    // append cmd->data after parent
	void * ( *append ) ( stone_node_t *, stone_node_t * );
    // store name and data to cmd->data and create node if cmd->data is null, and willnot dup name assuming name is dynamic
	void * ( *save ) ( ngx_command_t *cmd, stone_node_t *node, char  * name, void * value );
	// getch current node and set a node
	stone_node_t * ( *get ) ( ngx_command_t *cmd ); 
	void * ( *set ) ( ngx_command_t *cmd, stone_node_t *node ); 
	void * ( *child ) ( ngx_command_t *cmd ); // change cmd->data to child
    // change cmd->data to next
	void * ( *next ) ( ngx_command_t *cmd ); 
	void * ( *parent ) ( ngx_command_t *cmd ); // change cmd->data to parent
	void * ( *toString ) ( ngx_command_t *cmd, stone_node_t *root ); // root node to string
	void * ( *fromString ) ( ngx_command_t *cmd, char *text ); // root node to string
	// get type of a node
	enum stone_node_type ( *type ) ( stone_node_t *node );
	char * ( *name ) ( stone_node_t *node );
	void * ( *value ) ( stone_node_t *node );
	stone_node_t * ( *search ) ( stone_node_t *node, char *name );
};

	stone_node_t * mjson_create ( ngx_command_t *cmd, stone_node_t * root, enum stone_node_type type );
	void * mjson_append ( stone_node_t *, stone_node_t * );
	void * mjson_save ( ngx_command_t *cmd, stone_node_t *node, char  * name, void * value );
	stone_node_t * mjson_get ( ngx_command_t *cmd ); 
	void * mjson_set ( ngx_command_t *cmd, stone_node_t * ); 
	void * mjson_child ( ngx_command_t *cmd );
	void * mjson_next ( ngx_command_t *cmd );
	void * mjson_parent ( ngx_command_t *cmd );
	void * mjson_toString ( ngx_command_t *cmd, stone_node_t *root );
	void * mjson_fromString ( ngx_command_t *cmd, char *text );
	enum stone_node_type mjson_type ( stone_node_t *node );
	char * mjson_name ( stone_node_t *node );
	void * mjson_value ( stone_node_t *node );
	stone_node_t * mjson_search ( stone_node_t *node, char *name );

static stone_node_command_t node_mjson_command = {
	.create = mjson_create,
	.append = mjson_append,
	.save   = mjson_save,
	.get = mjson_get,
	.set = mjson_set,
	.child  = mjson_child,
	.next   = mjson_next,
	.parent = mjson_parent,
	.toString = mjson_toString,
    .fromString = mjson_fromString,
	.type = mjson_type,
	.name = mjson_name,
	.value = mjson_value,
	.search = mjson_search,
};

#endif
