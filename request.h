#ifndef STONE_REQUEST
#define STONE_REQUEST

#include "nginx.h"
#include "unistd.h"

#define GET 1
#define POST 2
#define PUT 3
#define HEAD 4
#define DELETE 5

#define _GET(name) ( main_get_variable ( server->req->get, name ) )
#define _POST(name) ( main_get_variable ( server->req->post, name ) )
#define _COOKIE(name) ( main_get_variable ( server->req->cookie, name ) )
#define _FILE(name) ( main_get_variable ( server->req->file, name ) )

struct stone_request_s {
	char ** path;
	struct list_head *get;
	struct list_head *post;
	struct list_head *cookie;
	struct list_head *file;
	struct stone_server_env *env;
};
typedef struct stone_request_s stone_request_t;


struct stone_file_s {
	char *filename;
	// path to save file data
	char *filepath;
	uint len;
};
typedef struct stone_file_s stone_file_t;

struct stone_server_env {
	char *Boundary;
	char *ContentType;
	uint BoundaryLen;
	uint ContentLength;
};

int main_add_variable( ngx_pool_t *pool, struct list_head *head, char *Name, void *Value);
void *main_get_variable( struct list_head *head, char *lpszName );
void *main_pop_variable( struct list_head *head, char *lpszName ); // pop up a value of name
int main_update_variable( struct list_head *head, char *lpszName, void *value );
stone_request_t *request_empty(ngx_pool_t *pool);
//int stone_request_init(ngx_pool_t *pool, stone_request_t *);
void main_store_data_rfc1867( ngx_pool_t *pool, stone_request_t *req, char *query );
void main_store_data( ngx_pool_t *pool, struct list_head *head, char *query );

#endif
