#include "hiredis/hiredis.h"
#include "notifier.h"
#include "app.h"
#include "opool.h"

int redis_pool_init(void ** ptr, opool_t *opool){
	struct timeval tv = {1, 500000};
    redisContext *redis;
	redis = redisConnectWithTimeout("127.0.0.1", 6379, tv );
	if ( redis->err ) {
		printf("redis not enabled");
        return -1;
	}
    *ptr = redis;
    return 0;
}

int redis_pool_destroy(void **ptr, opool_t *opool)
{
    if(*ptr)
        redisFree(*ptr);
    return 0;
}

int redis_init(notifier_block *nb, unsigned long ev, void *d){
    struct GLOBAL_RAPHTERS *data;
    opool_t *opool;
    data = (struct GLOBAL_RAPHTERS*)d;
    opool = opool_create(data->pool, OPOOL_FLAG_LOCK, 4, redis_pool_init, redis_pool_destroy);
    if(!opool) return NOTIFY_STOP;
    data->redis = opool;
    return NOTIFY_DONE;
}

int redis_deinit(notifier_block *nb, unsigned long ev, void *d){
    struct GLOBAL_RAPHTERS *data = d;
    if(data->redis){
        opool_destroy(data->redis);
    }
    return NOTIFY_DONE;
}

int module_redis_init(void){
    triger_set(NOTIFIER_INIT, 1, redis_init);
    triger_set(NOTIFIER_EXIT, 8, redis_deinit);
    return 0;
}
