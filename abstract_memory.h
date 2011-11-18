#ifndef __ABSTRACT_MEMORY_H__
#define __ABSTRACT_MEMORY_H__

#include "common.h"
#include "statistics.h"

struct AbstractMemory;
typedef struct AbstractMemory AbstractMemory;

typedef struct AbstractMemoryOps
{
    AbstractMemory *(*free)(AbstractMemory *m);
    void (*read)(AbstractMemory *m, memaddr_t addr, int size, MemoryCell *dst);
    void (*write)(AbstractMemory *m, memaddr_t addr, int size, const MemoryCell *src);
    void (*reveal)(AbstractMemory *m, memaddr_t addr, int size, const MemoryCell *src);
} AbstractMemoryOps;

struct AbstractMemory
{
    AbstractMemoryOps *ops;
    StatisticsInfo *info;
};

#endif
