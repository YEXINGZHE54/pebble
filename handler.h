#ifndef STONE_HANDLER
#define STONE_HANDLER

#include <fcgiapp.h>
#include <regex.h>
#include "app.h"

struct handler {
    void (*func)(stone_server_t *);
    int method;
    const char *regex_str;
    regex_t regex;
    size_t nmatch;
	struct handler *next;
};
typedef struct handler handler;

void error_handler(const char *msg, struct FCGX_Request *);
//void (*error_handler)(const char *) = default_error_handler;
int handler_init(void);
//void cleanup_handlers(ngx_pool_t *);
void add_handler(handler *);
handler *get_handler_head( void );

#endif
