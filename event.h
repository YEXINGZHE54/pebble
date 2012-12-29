#ifndef STONE_EVENT_H
#define STONE_EVENT_H
#include <event2/event.h>

typedef struct {
    struct event *ev;
    void * data;
} stone_event;

#endif
