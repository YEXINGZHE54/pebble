#ifndef STONE_RESPONSE
#define STONE_RESPONSE

#define ECHO(text) response_write(server->res, server->pool, text)
#define HEADER(name,value) response_add_header(server->res, server->pool, name, value)
#define _SCOOKIE( name, value ) setcookie (server, name, value, 0)
#define _DCOOKIE( name, value ) setcookie (server, name, value, -1)

struct header {
    char *name;
    char *value;
    struct header *next;
};
typedef struct header header;

struct text_segment {
    char *text;
    struct text_segment *next;
};
typedef struct text_segment text_segment;

struct stone_response_s {
    header *header_head;
    header *header_tail;
    text_segment *segment_head;
    text_segment *segment_tail;
};
typedef struct stone_response_s stone_response_t;

#include "app.h"

void response_write(stone_response_t *res, ngx_pool_t *pool, char *);
void response_add_header(stone_response_t *res, ngx_pool_t *pool, char *, char *);
//void response_send(stone_response_t *);
//stone_response_t *response_empty(ngx_pool_t *pool);

#endif
