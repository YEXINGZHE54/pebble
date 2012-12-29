#ifndef STONE_THREAD_H
#define STONE_THREAD_H

#include <pthread.h>
#include <fcgiapp.h>
#include <mysql/mysql.h>
#include <event2/event.h>
#include "nginx.h"
#include "connection.h"
#include "opool.h"

// Thread info
struct thread_info_t
{
    int receive_pipe;
    int send_pipe;
    struct event_base *base;
    opool_t *ev;
    opool_t *con_que;
    opool_t *connections;
    //struct stone_conn_que con_que;
    int nThreadID;
    pthread_t *pThread;
    pthread_mutex_t *lock;
    // FastCGI Request data
    //FCGX_Request *fcgi_request;
    ngx_pool_t *pool;
    ngx_pool_t *reuse;
    MYSQL *mysql;
    void *redis;
};

#endif
