#ifndef STONE_ASYNC_H
#define STONE_ASYNC_H

int async_register(stone_server_t *s, int fd, int flag, event_callback_fn callback_fn);
void async_schedule(void *d);

typedef struct stone_async_s stone_async_t;
typedef struct stone_async_thread stone_async_thread;
typedef int (*async_callfn)(int, void*);
typedef void (*async_setfn)(stone_async_t*);
struct stone_async_s{
    int fd;
    void *data;
    async_callfn fn;
    ngx_pool_t *pool;
};

struct stone_async_thread{
    ngx_pool_t *pool;
    opool_t *que;//no lock,resize
    ngx_pool_t *use;
    ngx_pool_t *back;
    pthread_t *pThread;
    pthread_mutex_t lock;
};

int async_thread_init(void *);
void * async_thread_start(void *);
int async_delegate(int fd, void *data, async_callfn fn, async_setfn datafn);

#endif
