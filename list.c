#include "list.h"
#include <stdio.h>

uint hasht_hash ( hasht_table *table, char *str, uint len )
{
    char *start, *end;
    uint hash=0;
    start = str;
    end = start + len;
    while ( ( start < end ) && *start )
    {
        hash += *start;
        start++;
    }
    hash = hash & (table->mask-1);
    return hash;
}

hasht_table *hasht_init ( ngx_pool_t *pool, uint mask )
{
    struct hlist_head *hhead;
    hasht_table *table;
    hhead = ngx_palloc ( pool, mask * sizeof ( *hhead ) );
    if ( !hhead ) return NULL;
    table = ngx_palloc ( pool, sizeof ( *table ) );
    if ( !table ) return NULL;
    table->num = 0;
    table->mask = mask;
    table->head = hhead;
    return table;
}

void *hasht_find ( hasht_table *table, char *str )
{
    if ( !str ) return NULL;
    uint hash = 0, slen;
    struct hlist_node *pos;
    hasht_node *tpos;
    slen = strlen ( str );
    hash = hasht_hash ( table, str, slen );
    if ( hash >= table->mask ) return NULL;
    struct hlist_head *head = table->head + hash;
    hlist_for_each_entry(tpos, pos, head, ptr)
    {
        if ( strncmp ( tpos->name, str, slen ) == 0 )
        {
            return tpos->data;
        }
    }
    return NULL;
}

int hasht_resize ( ngx_pool_t *pool, hasht_table **otable )
{
    hasht_table *table = *otable;
    if ( table->num <= table->mask ) return 0;
    uint n, hash;
    struct hlist_head *hhead;
    hasht_table *newtable;
    struct hlist_node *pos;
    hasht_node *tpos;
    newtable = hasht_init ( pool, 2 );
    if ( !newtable ) return -1;
    for ( n=0; n< table->mask; n++ )
    {
        hhead = table->head + n;
        hlist_for_each_entry(tpos, pos, hhead, ptr)
        {
            hash = hasht_hash ( newtable, tpos->name, strlen ( tpos->name ) );
            hlist_add_head ( &( tpos->ptr ), newtable->head + hash );
        }
    }
    *otable = newtable;
}

int hasht_insert ( ngx_pool_t *pool, hasht_table *table, char *name, void *value )
{
    if ( !name ) return -1;
    uint hash, slen;
    hasht_node *node;
    slen = strlen ( name );
    hash = hasht_hash ( table, name, slen );
    if ( hash >= table->mask ) return -1;
    node = ngx_palloc ( pool, sizeof ( *node ) );
    if ( !node ) return -1;
    node->name = name;
    node->data = value;
    hlist_add_head ( &( node->ptr ), table->head + hash );
    table->num++;
    hasht_resize( pool, &table );
    return 0;
}

int hasht_delete ( hasht_table *table, char *name )
{
    if ( !name ) return -1;
    uint hash, slen;
    hasht_node *tpos;
    struct hlist_node *pos;
    slen = strlen ( name );
    hash = hasht_hash ( table, name, slen );
    if ( hash >= table->mask ) return -1;
    struct hlist_head *head = table->head + hash;
    hlist_for_each_entry(tpos, pos, head, ptr)
    {
        if ( strncmp ( tpos->name, name, slen ) == 0 )
        {
            hlist_del_init ( pos );
            table->num--;
            return 0;
        }
    }
    return -1;
}

int hasht_update (ngx_pool_t *pool, hasht_table *table, char *name, void *value )
{
    if ( !name ) return -1;
    uint hash, slen;
    hasht_node *tpos, *node;
    struct hlist_node *pos;
    slen = strlen ( name );
    hash = hasht_hash ( table, name, slen );
    if ( hash >= table->mask ) return -1;
    struct hlist_head *head = table->head + hash;
    hlist_for_each_entry(tpos, pos, head, ptr)
    {
        if ( strncmp ( tpos->name, name, slen ) == 0 )
        {
            tpos->data = value;
            return 0;
        }
    }
    node = ngx_palloc ( pool, sizeof ( *node ) );
    if ( !node ) return -1;
    node->name = name;
    node->data = value;
    hlist_add_head ( &( node->ptr ), head );
    table->num++;
    hasht_resize( pool, &table );
    return 0;
}
