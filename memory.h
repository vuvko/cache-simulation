#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "abstract_memory.h"
#include "parsecfg.h"

#include <stdio.h>

AbstractMemory *memory_create(
    ConfigFile *cfg, 
    const char *var_prefix, 
    StatisticsInfo *info);
void mem_dump(AbstractMemory *a, FILE *f);

#endif
