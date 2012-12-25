#include "opool.h"

opool_t * opool_create(ngx_pool_t *pool, unsigned int n, opool_fn init_fn)
{
    opool_t *opool;
    opool_node_t *node, *prev;
    int i,rc;
    opool = ngx_palloc(pool, sizeof(*opool) + n*sizeof(*node));
    if(!opool) return NULL;
    pthread_mutex_init(&opool->mutex, NULL);
    opool->mem = pool;
    node = ((void*)opool + sizeof(*opool));
    opool->free = node;
    opool->busy = NULL;
    rc = init_fn(&node->data, opool);
    if(rc != 0) goto failed;
    prev = node;
    for(i=1;i<n;i++){
        node += sizeof(*node);
        rc = init_fn(&node->data, opool);
        if(rc != 0) goto failed;
        prev->next = node;
        prev = node;
    }
    node->next = NULL;
    return opool;
failed:
    ngx_pfree(pool, opool);
    return NULL;
}

int opool_destroy(ngx_pool_t *pool, opool_t*opool, opool_fn destroy_fn)
{
    opool_node_t *node;
    int rc=0;
    node =(opool_node_t*)(opool + sizeof(*opool));
    if(destroy_fn){
        do{
            rc = destroy_fn(&node->data, opool);
            if(rc!=0) break;
        }while(node=node->next);
    }
    ngx_pfree(pool, opool);
    return rc;
}

void * opool_request(opool_t *opool)
{
    opool_node_t *free;
    pthread_mutex_lock(&opool->mutex);
    free = opool->free;
    if(free == NULL) {
        pthread_mutex_unlock(&opool->mutex);
        return NULL;
    }
    opool->free = free->next;
    free->next=opool->busy;
    opool->busy=free;
    pthread_mutex_unlock(&opool->mutex);
    return free->data;
}

int opool_release(opool_t *opool, void *data)
{    
    opool_node_t *busy;
    pthread_mutex_lock(&opool->mutex);
    busy = opool->busy;
    if(busy == NULL) {
        pthread_mutex_unlock(&opool->mutex);
        return -1;
    }
    opool->busy = busy->next;
    busy->next=opool->free;
    opool->free=busy;
    busy->data = data;
    pthread_mutex_unlock(&opool->mutex);
    return 0;
}
