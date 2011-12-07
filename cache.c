#include "cache.h"
#include "direct_cache.h"
#include "full_cache.h"

#include <string.h>

AbstractMemory *
cache_create(ConfigFile *cfg, const char *var_prefix, StatisticsInfo *info, AbstractMemory *mem, Random *rnd)
{
    const char *a = config_get(cfg, "associativity");
    if (!a) {
        error_undefined("associativity");
    } else if (!strcmp(a, "full")) {
        return full_cache_create(cfg, var_prefix, info, mem, rnd);
    } else if (!strcmp(a, "direct")) {
        return direct_cache_create(cfg, var_prefix, info, mem, rnd);
    } else {
        error_invalid("associativity");
    }
    return NULL;
}
