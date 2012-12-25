#include "handler.h"
#include "notifier.h"

handler *head = NULL;
handler *last = NULL;

void error_handler(const char *msg, struct FCGX_Request *fcgx) {
    FCGX_FPrintF( fcgx->out, "content-type: text/html\n\nerror: %s\n", msg );
}

int cleanup_handlers(notifier_block *nb, unsigned long ev, void *d);

int module_handler_init(void) {
	handler *cur = head;
    while (cur != NULL) {
        if (regcomp(&cur->regex, cur->regex_str, 0) != 0) {
            return -1;
        }
        cur = cur->next;
    }
    triger_set(NOTIFIER_EXIT, 8, cleanup_handlers);
    return 0;
}

int cleanup_handlers(notifier_block *nb, unsigned long ev, void *d) {
	handler *cur = head;
    while (cur != NULL) {
        regfree(&cur->regex);
        cur = cur->next;
    }
    return NOTIFY_DONE;
}

void add_handler(handler *h) {
    if (head == NULL) {
        head = h;
    } else {
        last->next = h;
    }
    last = h;
}

handler * get_handler_head( void )
{
	return head;
}
