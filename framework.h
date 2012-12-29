#ifndef _FRAMEWORK_H
#define _FRAMEWORK_H


#include <stdlib.h>
#include <string.h>
#include "request.h"
#include "response.h"
#include "nginx.h"
#include "appsql.h"
#include "command.h"
#include "session.h"
#include "cache.h"
#include "handler.h"
#include "tpl.h"
#include "app.h"

void atquit(void);
void worker_process(void *data);
int spawn_worker(void *data, void (*proc)(void *));
void dispatch(ngx_pool_t *pool);
#endif
