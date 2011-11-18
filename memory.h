#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "abstract_memory.h"
#include "parse_config.h"

AbstractMemory *memory_create(ConfigFile *cfg, const char *var_prefix, StatisticsInfo *info);

#endif
