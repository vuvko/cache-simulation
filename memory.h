#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "abstract_memory.h"
#include "parsecfg.h"

AbstractMemory *memory_create(ConfigFile *cfg, const char *var_prefix, StatisticsInfo *info);

#endif
