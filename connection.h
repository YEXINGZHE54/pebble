#ifndef STONE_CONNECTION_H
#define STONE_CONNECTION_H

#include <fcgiapp.h>
#include <pthread.h>
#include "list.h"
struct stone_conn {
    struct FCGX_Request fcgx;
    struct list_head head;
};

struct stone_conn_que {
    pthread_mutex_t lock;
    struct list_head head;
};

typedef struct stone_conn stone_conn;

#endif
