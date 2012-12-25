#include "notifier.h"
#include "app.h"
#include <unistd.h>

// the smallest, first called
static int notifier_chain_register ( struct notifier_block ** nl, struct notifier_block * nb )
{
    while ( ( *nl ) != NULL )
    {
        if ( nb->priority <= ( *nl )->priority )
            break;
        nl =&( ( *nl )->next );
    }
    nb->next = *nl;
    *nl = nb;
    return 0;
}

static int notifier_chain_unregister ( struct notifier_block **nl, struct notifier_block *nb )
{
    while ( ( *nl ) != NULL )
    {
        if ( ( *nl ) == nb )
        { 
            *nl = nb->next;
            return 0;
        }
        nl = &( ( *nl )->next );
    }
    return -1;
}

static int notifier_chain_call ( struct notifier_block **nl, unsigned long ev, void *data, int total, int *affected )
{
    struct notifier_block *nb;
    int ret = NOTIFY_DONE;
    nb = *nl;
    while ( nb && total )
    {
        ret = nb->call ( nb, ev, data );
        if ( affected ) *affected = *affected + 1;
        if ( ( ret & NOTIFIER_STOP ) == NOTIFIER_STOP ) break;
        total--;
        nb = nb->next;
    }
    return ret;
}

void notifier_chain_init ( void )
{
    memset ( stone_notifier_chain, 0, sizeof ( stone_notifier_chain ) );
}

int triger_set ( enum notifier_event ev, uint priority, notifier_callback call )
{
    struct notifier_block *nb;
    struct raw_notifier_head *nh; 
    nb = ngx_palloc ( globals_r.pool, sizeof ( *nb ) );
    if ( !nb ) return -1;
    nb->call = call;
    nb->priority = priority;
    nh = &stone_notifier_chain[ev];
    notifier_chain_register ( & ( nh->head ), nb );
    return 0;
}

int triger_unset ( enum notifier_event ev, notifier_callback call )
{
    struct notifier_block **nl;
    struct raw_notifier_head nh; 
    nh = stone_notifier_chain[ev];
    nl = & ( nh.head );
    while ( ( *nl ) != NULL )
    {
        if ( ( *nl )->call == call )
        { 
            *nl = ( *nl )->next;
            return 0;
        }
        nl = &( ( *nl )->next );
    }
    return -1;
}

int triger ( enum notifier_event ev, void * data )
{
    struct notifier_block **nl;
    struct raw_notifier_head nh;
    int ret;
    nh = stone_notifier_chain[ev];
    nl = & ( nh.head );
    ret = notifier_chain_call ( nl, ev, data, -1, NULL );
    return ret;
}
