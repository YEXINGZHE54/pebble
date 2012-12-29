#include "opool.h"

opool_t * opool_create(ngx_pool_t *pool, unsigned int flag, unsigned int n, opool_fn init_fn, opool_fn destroy_fn)
{
    opool_t *opool;
    opool_node_t *node, *prev;
    int i,rc = 0;
    if ( n < 0 ) return NULL;
    opool = ngx_palloc(pool, sizeof(*opool) + n*sizeof(*node));
    if(!opool) return NULL;
    if (flag & OPOOL_FLAG_LOCK)
        pthread_mutex_init(&opool->mutex, NULL);
    opool->mem = pool;
    opool->busy = NULL;
    opool->mode = flag;
    opool->destroy_fn = destroy_fn;
    opool->init_fn = init_fn;
    if(n==0) 
    {
        opool->free = NULL;
        goto finished;
    }
    //here n >= 1
    node = ((void*)opool + sizeof(*opool));
    opool->free = node;
    //init nodes
    if(init_fn){
        rc = init_fn(&node->data, opool);
        if(rc != 0) goto failed;
    }
    prev = node;
    for(i=1;i<n;i++){
        node = (void*)node + sizeof(*node);
        if (init_fn) {
            rc = init_fn(&node->data, opool);
            if(rc != 0) goto failed;
        }
        prev->next = node;
        prev = node;
    }
    node->next = NULL;
finished:
    return opool;
failed:
    opool_destroy(opool);
    //ngx_pfree(pool, opool);
    return NULL;
}

int opool_destroy(opool_t *opool)
{
    opool_node_t *node;
    ngx_pool_t *pool;
    int rc=0;
    opool_fn destroy_fn;
    pool = opool->mem;
    destroy_fn = opool->destroy_fn;
    //node =(opool_node_t*)(opool + sizeof(*opool));
    if(destroy_fn){
        node = opool->free;
        while(node){
            rc = destroy_fn(&node->data, opool);
            if(rc!=0) break;
            node = node->next;
        };
        node = opool->busy;
        while(node){
            rc = destroy_fn(&node->data, opool);
            if(rc!=0) break;
            node = node->next;
        };
    }
    ngx_pfree(pool, opool);
    return rc;
}

void * opool_request(opool_t *opool)
{
    opool_node_t *free;
    int rc = 0;
    if (opool->mode & OPOOL_FLAG_LOCK)pthread_mutex_lock(&opool->mutex);
    free = opool->free;
    if(free == NULL) {
        if(opool->mode & OPOOL_FLAG_RESIZE)
        {
            free = ngx_palloc(opool->mem, sizeof(*free));
            if(free == NULL) goto failed;
            if(opool->init_fn){
                rc = opool->init_fn(&(free->data), opool);
                if(rc != 0) goto failed;
            }
            free->next = NULL;
        }else{
failed:
        if(opool->mode & OPOOL_FLAG_LOCK) pthread_mutex_unlock(&opool->mutex);
        return NULL;
        }
    }
    opool->free = free->next;
    free->next=opool->busy;
    opool->busy=free;
    if(opool->mode & OPOOL_FLAG_LOCK) pthread_mutex_unlock(&opool->mutex);
    return free->data;
}

opool_iter *opool_requestAll(opool_t *p)
{
    opool_node_t *free = NULL;
    if (p->mode & OPOOL_FLAG_LOCK) pthread_mutex_lock(&p->mutex);
    free = p->free;
    p->free = NULL;
    if (p->mode & OPOOL_FLAG_LOCK) pthread_mutex_unlock(&p->mutex);
    return (opool_iter *)free;
}

void opool_releaseAll(opool_t *p, opool_iter *iter)
{
    opool_node_t *free = NULL;
    if (p->mode & OPOOL_FLAG_LOCK) pthread_mutex_lock(&p->mutex);
    free = p->free;
    if(free == NULL){
        p->free = iter;
        goto unlock_release;
    }
    //find the end of free list
    while(free->next) free = free->next;
    free->next = iter;
unlock_release:
    if (p->mode & OPOOL_FLAG_LOCK) pthread_mutex_unlock(&p->mutex);
    return; 
}


int opool_release(opool_t *opool, void *data)
{    
    opool_node_t *busy;
    int rc = 0;
    if(opool->mode & OPOOL_FLAG_LOCK) pthread_mutex_lock(&opool->mutex);
    busy = opool->busy;
    if(busy == NULL) {
        if(opool->mode & OPOOL_FLAG_RESIZE)
        {
            busy = ngx_palloc(opool->mem, sizeof(*busy));
            if(busy == NULL) goto failed;
            if(opool->init_fn){
                rc = opool->init_fn(&(busy->data), opool);
                if(rc != 0) goto failed;
            }
            busy->next = NULL;
        }else{
failed:
        if(opool->mode & OPOOL_FLAG_LOCK)
            pthread_mutex_unlock(&opool->mutex);
        return -1;
        }
    }
    opool->busy = busy->next;
    busy->next=opool->free;
    opool->free=busy;
    busy->data = data;
    if(opool->mode & OPOOL_FLAG_LOCK) pthread_mutex_unlock(&opool->mutex);
    return 0;
}
