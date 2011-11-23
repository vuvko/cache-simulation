#ifndef __FULL_CACHE_H__
#define __FULL_CACHE_H__

#include "abstract_memory.h"
#include "parsecfg.h"
#include "random.h"

AbstractMemory *full_cache_create(ConfigFile *cfg, 
                                  const char *var_prefix, 
                                  StatisticsInfo *info, 
                                  AbstractMemory *mem, 
                                  Random *rnd);

#endif
