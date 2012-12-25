#ifndef STONE_OPOOL_H
#define STONE_OPOOL_H

#include "nginx.h"
#include <pthread.h>

typedef struct opool_s opool_t;
typedef struct opool_node_s opool_node_t;
typedef int (*opool_fn)(void **, opool_t*);

struct opool_s{
    opool_node_t *busy;
    opool_node_t *free;
    ngx_pool_t *mem;
    pthread_mutex_t mutex;
};

struct opool_node_s{
    void *data;
    opool_node_t *next;
};

opool_t * opool_create(ngx_pool_t *pool, unsigned int n, opool_fn init_fn);
int opool_destroy(ngx_pool_t *pool, opool_t*opool, opool_fn destroy_fn);
void * opool_request(opool_t *opool);
int opool_release(opool_t *opool, void *data);
#endif
