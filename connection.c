#include "notifier.h"
#include "threadinfo.h"
#include <fcgiapp.h>

int connection_pool_init(void **ptr, opool_t *p)
{
    *ptr = ngx_palloc(p->mem, sizeof(struct FCGX_Request));
    if(*ptr == NULL) return -1;
    return 0;
}

int connection_pool_destroy(void **ptr, opool_t *p){}

int connection_init(notifier_block *nb, unsigned long ev, void *d)
{
    //struct GLOBAL_RAPHTERS *g = d;
    struct thread_info_t *t = d;
    t->connections = opool_create(t->pool, OPOOL_FLAG_RESIZE, 10, connection_pool_init, NULL);
    if(t->connections == NULL) return NOTIFY_STOP;
    t->con_que = opool_create(t->pool, OPOOL_FLAG_RESIZE|OPOOL_FLAG_LOCK, 10, NULL, NULL);
    if(t->con_que == NULL) return NOTIFY_STOP;
    return NOTIFY_DONE;
}

int connection_exit(notifier_block *nb, unsigned long ev, void *d)
{
    //struct GLOBAL_RAPHTERS *g = d;
    struct thread_info_t *t = d;
    if(t->connections)
        opool_destroy(t->connections);
    if(t->con_que)
        opool_destroy(t->con_que);
    return NOTIFY_DONE;
}

int module_connection_init(void)
{
   triger_set(NOTIFIER_THREAD, 9, connection_init);
   triger_set(NOTIFIER_THREAD_EXIT, 0, connection_exit);
   return 0;
}
