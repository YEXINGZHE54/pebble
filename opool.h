#ifndef STONE_OPOOL_H
#define STONE_OPOOL_H

#include "nginx.h"
#include <pthread.h>

typedef struct opool_s opool_t;
typedef struct opool_node_s opool_node_t;
typedef opool_node_t opool_iter;
typedef int (*opool_fn)(void **, opool_t*);
#define OPOOL_FLAG_DEFAULT  0x00
#define OPOOL_FLAG_LOCK     0x01
#define OPOOL_FLAG_RESIZE   0x02

struct opool_s{
    opool_node_t *busy;
    opool_node_t *free;
    ngx_pool_t *mem;
    pthread_mutex_t mutex;
    opool_fn init_fn;
    opool_fn destroy_fn;
    unsigned int mode;
};

struct opool_node_s{
    void *data;
    opool_node_t *next;
};

opool_t * opool_create(ngx_pool_t *pool, unsigned int flag, unsigned int n, opool_fn init_fn, opool_fn destroy_fn);
int opool_destroy(opool_t*opool);
void * opool_request(opool_t *opool);
opool_iter *opool_requestAll(opool_t *p);
void opool_releaseAll(opool_t *p, opool_iter *iter);
int opool_release(opool_t *opool, void *data);
#endif
