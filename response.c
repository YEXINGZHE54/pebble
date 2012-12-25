#include "response.h"
#include "notifier.h"

int response_start(notifier_block *nb, unsigned long ev, void *d);
int response_send(notifier_block *nb, unsigned long ev, void *d);
int module_response_init(void){
    triger_set(NOTIFIER_START, 5, response_start);
    triger_set(NOTIFIER_RESPONSE, 5, response_send);
    return 0;
}

stone_response_t *response_empty(ngx_pool_t *pool) {
    stone_response_t *result = ngx_palloc(pool, sizeof(stone_response_t));
    result->header_head = NULL;
    result->header_tail = NULL;
    result->segment_head = NULL;
    result->segment_tail = NULL;
    return result;
}

int response_start(notifier_block *nb, unsigned long ev, void *d)
{
    stone_server_t *server = d;
    stone_response_t *res;
    res = response_empty(server->pool);
    if(res == NULL)
        return NOTIFY_STOP;
    server->res = res;
    return NOTIFY_DONE;
}

void response_write(stone_response_t *res, ngx_pool_t *pool, char *text) {
// 	stone_response_t *res = server->res;
// 	ngx_pool_t *pool = server->pool;
    text_segment *segment = ngx_palloc(pool, sizeof(text_segment));
    segment->text = ngx_strdup(pool, text);
    segment->next = NULL;
    if (res->segment_head == NULL) {
        res->segment_head = segment;
    } else {
        res->segment_tail->next = segment;
    }
    res->segment_tail = segment;
}

void response_add_header(stone_response_t *res, ngx_pool_t *pool, char *name, char *val) {
// 	stone_response_t *res = server->res;
// 	ngx_pool_t *pool = server->pool;
    header *h = ngx_palloc(pool, sizeof(header));
    h->name = ngx_strdup(pool, name);
    h->value = ngx_strdup(pool, val);
    h->next = NULL;
    if (res->header_head == NULL) {
        res->header_head = h;
    } else {
        res->header_tail->next = h;
    }
    res->header_tail = h;
}

int response_send(notifier_block *nb, unsigned long ev, void *d) {
    stone_response_t *res;
    stone_server_t *server = d;
    header *cur_h;
    res = server->res;
    for (cur_h = res->header_head; cur_h != NULL;) {
        //printf("%s: %s\n", cur_h->name, cur_h->value);
		FCGX_FPrintF( server->thread->fcgi_request->out, "%s: %s\n", cur_h->name, cur_h->value);
//         free(cur_h->name);
//         free(cur_h->value);

        header *next = cur_h->next;
//         free(cur_h);
        cur_h = next;
    }
    //printf("\n");
    FCGX_PutS( "\n", server->thread->fcgi_request->out );
    text_segment *cur_s;
    for (cur_s = res->segment_head; cur_s != NULL;) {
        //printf("%s", cur_s->text);
		FCGX_PutS( cur_s->text, server->thread->fcgi_request->out );
//         free(cur_s->text);

        text_segment *next = cur_s->next;
//         free(cur_s);
        cur_s = next;
    }
    return NOTIFY_DONE;
//     free(res);
}
