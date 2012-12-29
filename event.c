#include <event2/event.h>
#include "notifier.h"
#include "threadinfo.h"
#include "event.h"

extern void dispatch_thread_que(struct thread_info_t *);
extern void thread_pipe_handler(int, short, void *);

int event_pool_init(void **ptr, opool_t *p){
    stone_event *ev;
    ev = ngx_palloc(p->mem, sizeof(*ev));
    if(!ev) return -1;
    ev->ev = event_new(NULL, 0, 0, NULL, NULL);
    if(ev->ev == NULL) return -1;
    *ptr = ev;
    return 0;
}

int event_pool_destroy(void **ptr, opool_t *p){
    if(*ptr){
        stone_event *ev = *ptr;
        event_free(ev->ev);
    }
        //event_free(*ptr);
    return 0;
}

int event_start(notifier_block *nb, unsigned long ev, void *d){
    struct thread_info_t *t = d;
    t->base = event_base_new();
    if ( t->base == NULL ) return NOTIFY_STOP;
    t->ev = opool_create(t->pool, OPOOL_FLAG_RESIZE, 3, event_pool_init, event_pool_destroy);
    if (t->ev == NULL) return NOTIFY_STOP;
    //t->pev = event_new(t->base, t->receive_pipe, EV_READ | EV_PERSIST, thread_pipe_handler, t);
    //if(t->pev == NULL ) return NOTIFY_STOP;
    //event_add(t->pev, 0);
    return NOTIFY_DONE;
}

int event_stop(notifier_block *nb, unsigned long ev, void *d){
    struct thread_info_t *t = d;
    opool_destroy(t->ev);
    //event_free(t->pev);
    event_base_free(t->base);
    return;
}

int module_event_init(void){
    triger_set(NOTIFIER_THREAD, 9, event_start);
    triger_set(NOTIFIER_THREAD_EXIT, 0, event_stop);
    return 0;
}
