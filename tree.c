#include "tree.h"
#include "json.h"
#include <assert.h>

// create a new node, and append it to root if root is a node
stone_node_t * mjson_create ( ngx_command_t *cmd, stone_node_t * root, enum stone_node_type type )
{
	json_t *cur, *parent;
	switch ( type )
	{
		case NODE_NC:
			cur = json_new_string ( cmd->pool, ngx_strdup( cmd->pool, "NC" ) );
			break;
		case NODE_CONDITION:
			cur = json_new_string ( cmd->pool, ngx_strdup( cmd->pool, "IF" ) );
			break;
		case NODE_VALUE:
			cur = json_new_string ( cmd->pool, ngx_strdup( cmd->pool, "1" ) );
			break;
		case NODE_ARR:
			cur = json_new_array( cmd->pool );
			break;
		case NODE_OBJ:
			cur = json_new_object( cmd->pool );
			break;
		default:
			return NULL;
	}

	if ( cur == NULL ) return NULL;
	
	if ( root != NULL )
	{
		parent = ( json_t * ) root;
		json_insert_child( parent, cur );
	}
	
// 	cmd->data = cur;
	
		return ( stone_node_t * )cur;
}

// append a node after parent
void * mjson_append ( stone_node_t *p, stone_node_t * c )
{
	json_t *cur, *parent;
	parent = ( json_t * ) p;
// 	parent = p->parent;
// 	if ( !parent ) return NULL;
	cur = (json_t * ) c;
	json_insert_child( parent, cur );
	return cur;
}

// save name and value in a node and create one if not exists
void * mjson_save ( ngx_command_t *cmd, stone_node_t *node, char  * name, void * value )
{
	json_t *cur, *child, *temp;

	if ( !node )
	{
		if ( !name ) name = ngx_strdup( cmd->pool, "NC" );
		temp = json_new_string ( cmd->pool, name );
		if ( !temp ) return NULL;
		cur = temp;
	}else{
		cur = (json_t * ) node;
		if ( name && *name )
			cur->text = name;
	}

	child = json_new_string ( cmd->pool, value );
	if ( !child ) return NULL;
	
	json_insert_child ( cur, child );

	return cur;
}

// get current of a node
stone_node_t * mjson_get ( ngx_command_t *cmd )
{
	json_t *cur;
	
	cur = ( json_t * )cmd->data;
	
	return (stone_node_t *)cur;
}

// set current node
void * mjson_set ( ngx_command_t *cmd, stone_node_t *node )
{
	cmd->data = node;
	
	return node;
}

// get child of a node
void * mjson_child ( ngx_command_t *cmd )
{
	json_t *cur, *child;

	cur = ( json_t * )cmd->data;
    if ( !cur ) return cur;
	if ( cur->type != JSON_ARRAY && cur->type != JSON_OBJECT ) return NULL;
	child = cur->child;
    if ( !child ) return NULL;
    // change cmd->data to next
    cmd->data = child;
	return child;
}

// get next one of a node, iterator
void * mjson_next ( ngx_command_t *cmd )
{
	json_t *cur, *next;

	cur = ( json_t * )cmd->data;
    if ( !cur ) return cur;
	next = cur->next;
    if ( !next ) return NULL;
    // change cmd->data to next
    cmd->data = next;
	return next;
}

// get parent of node
void * mjson_parent ( ngx_command_t *cmd )
{
	json_t *cur, *p;

	cur = ( json_t * )cmd->data;
    if ( !cur ) return cur;
	p = cur->parent;
    if ( !p ) return NULL;
    // change cmd->data to parent
    cmd->data = p;
	return p;
}

void * mjson_toString ( ngx_command_t *cmd, stone_node_t * root )
{
	char *text;
	json_t *tree;

	tree = ( json_t * ) root;

	ngx_json_tree_to_string(cmd->pool, tree, &text);

	return text;
}

void *mjson_fromString( ngx_command_t *cmd, char *text )
{
    json_t *root;
    int rc;
    rc = json_parse_document(cmd->pool, &root, text);
    if( rc != JSON_OK)
        return NULL;
    return root;
}

enum stone_node_type mjson_type ( stone_node_t *node )
{
	json_t *cur;
    assert ( node );
    cur = ( json_t * ) node;
    switch ( cur->type )
    {
		case JSON_ARRAY:
			return NODE_ARR;
			break;
		case JSON_STRING:
			if ( *(cur->text) == 'N' && *(cur->text + 1) == 'C' )
				return NODE_NC;
			else if ( *(cur->text) == 'I' && *(cur->text + 1) == 'F' )
				return NODE_CONDITION;
			else
				return NODE_VALUE;
			break;
		case JSON_OBJECT:
			return NODE_OBJ;
			break;
		default:
			return NODE_VALUE;
			break;
    }
}

char * mjson_name ( stone_node_t *node )
{
	json_t *cur;
	assert ( node );
	cur = ( json_t * ) node;
	return cur->text;
}

void * mjson_value ( stone_node_t *node )
{
	json_t *child, *cur;
	assert ( node );
	cur = ( json_t * ) node;
	child = ( json_t * ) cur->child;
	if (!child ) return NULL;
	return child->text;
}

stone_node_t * mjson_search ( stone_node_t *node, char *name )
{
	json_t *root, *child;
	assert ( name );
	int len = strlen ( name );
	root = ( json_t * ) node;
	child = root->child;
	while ( child )
	{
		if ( strncmp( child->text, name, len ) == 0 )
			return ( stone_node_t * )child;
		child = child->next;
	}
	return NULL;
}
