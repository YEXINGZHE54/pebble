#include <stdlib.h>
#include "module.h"
#include "module_config.h"
#include "error.h"

void module_init ( void )
{
    int i,rc;
    struct stone_module md;
    for ( i = 0; i < STONE_MODULE_MAX ; i++ )
    {
        md = stone_modules[i];
        rc = (*md.init)();
        if ( rc != 0 ) exit(STONE_ERROR_MODULE); //module error
    }
}
