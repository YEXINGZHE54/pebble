#ifndef STONE_MODULE_CONF_H
#define STONE_MODULE_CONF_H

#define MODULE_DECLARE(name) extern int name(void);

#define STONE_MODULE_MAX 8

struct stone_module {
    int ( *init ) (void);
};

MODULE_DECLARE(module_global_init);
MODULE_DECLARE(module_mysql_init);
MODULE_DECLARE(module_redis_init);
MODULE_DECLARE(module_tpl_init);
MODULE_DECLARE(module_handler_init);
MODULE_DECLARE(module_request_init);
MODULE_DECLARE(module_response_init);
MODULE_DECLARE(module_session_init);

static struct stone_module stone_modules[STONE_MODULE_MAX]  = \
{
    {.init = module_global_init},
    {.init = module_mysql_init},
    {.init = module_redis_init},
    {.init = module_tpl_init},
    {.init = module_handler_init},
    {.init = module_request_init},
    {.init = module_response_init},
    {.init = module_session_init},
};

#endif
