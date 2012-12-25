#ifndef STONE_THREAD_H
#define STONE_THREAD_H

#include <pthread.h>
#include <fcgiapp.h>
#include <mysql/mysql.h>
#include "nginx.h"

// Thread info
struct thread_info_t
{
    int nThreadID;
    pthread_t *pThread;
    pthread_mutex_t *lock;
    // FastCGI Request data
    FCGX_Request *fcgi_request;
    ngx_pool_t *pool;
    MYSQL *mysql;
};

#endif
