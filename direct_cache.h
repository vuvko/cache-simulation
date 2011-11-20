#ifndef DIRECT_CACHE_H_INCLUDED
#define DIRECT_CACHE_H_INCLUDED

#include "abstract_memory.h"
#include "parsecfg.h"
#include "random.h"

AbstractMemory *direct_cache_create(ConfigFile *cfg, 
                                    const char *var_prefix, 
                                    StatisticsInfo *info, 
                                    AbstractMemory *mem, 
                                    Random *rnd);

#endif
