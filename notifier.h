#ifndef STONE_NOTIFIER_H
#define STONE_NOTIFIER_H

#include <sys/types.h>

#define NOTIFY_DONE 0x0000
#define NOTIFY_STOP 0x8000
#define STONE_NOTIFIER_CHAIN_SIZE 8

typedef struct notifier_block notifier_block;
typedef int ( * notifier_callback ) ( notifier_block *, unsigned long, void * );
// copied from linux kernel
struct notifier_block {
//   int ( *call ) ( struct notifier_block *, unsigned long, void * );
    notifier_callback call;
    struct notifier_block *next;
    int priority;
};

struct raw_notifier_head {
    struct notifier_block *head;
};

static struct raw_notifier_head stone_notifier_chain[STONE_NOTIFIER_CHAIN_SIZE];

enum notifier_event { NOTIFIER_INIT = 0, NOTIFIER_THREAD, NOTIFIER_START, \
    NOTIFIER_TPLOUT, NOTIFIER_RESPONSE, NOTIFIER_STOP, \
        NOTIFIER_THREAD_EXIT, NOTIFIER_EXIT };

void notifier_chain_init ( void );
//set and unset is not thread_safe
int triger_set ( enum notifier_event ev, uint priority, notifier_callback call );
int triger_unset ( enum notifier_event ev, notifier_callback call );
//triger function is thread_safe or not, depending on data
int triger ( enum notifier_event ev, void * data );
//int notifier_chain_register ( struct notifier_block **, struct notifier_block * );
//int notifier_chain_unregister ( struct notifier_block **, struct notifier_block * );
//int notifier_chain_call ( struct notifier_block **, unsigned long, void * );

#endif
