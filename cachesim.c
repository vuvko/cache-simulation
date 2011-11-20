#include <stdio.h>
#include <stdlib.h>

#include "trace.h"
#include "common.h"
#include "memory.h"
#include "parsecfg.h"
#include "statistics.h"
#include "direct_cache.h"
#include "random.h"
#include "abstract_memory.h"

int
main(int argc, char *argv[])
{
    printf("!!!cfg\n\n\n");
    ConfigFile *cfg = config_read(argv[1]);
    printf("!!!\n\n\n");
    if (!cfg) {
        fprintf(stderr, "no cfg\n");
        return 1;
    }
    printf("!!!info\n\n\n");
    StatisticsInfo *info = statistics_create(cfg);
    printf("!!!\n\n\n");
    if (!info) {
        fprintf(stderr, "no info\n");
        return 1;
    }
    printf("!!!mem\n\n\n");
    AbstractMemory *mem = memory_create(cfg, "", info);
    printf("!!!\n\n\n");
    if (!mem) {
        fprintf(stderr, "no mem\n");
        return 1;
    }
    printf("!!!cache\n\n\n");
    AbstractMemory *cache = direct_cache_create(cfg, "", info, mem, NULL);
    printf("!!!\n\n\n");
    if (!cache) {
        fprintf(stderr, "no cache\n");
        return 1;
    }
    printf("!!!trace\n\n\n");
    Trace *t = trace_open("trace.fire", NULL);
    printf("!!!\n\n\n");
    if (!t) {
        fprintf(stderr, "no trace");
        return 1;
    }
    TraceStep *ts = NULL;
    while (trace_next(t)) {
        ts = trace_get(t);
        if (ts->mem == 'D') {
            cache->ops->reveal(cache, ts->addr, ts->size, ts->value);
        } else if (ts->op == 'I') {
            mem->ops->reveal(mem, ts->addr, ts->size, ts->value);
        }
    }
    printf("!!!trace\n\n\n");
    t = trace_open("trace.txt", NULL);
    printf("!!!\n\n\n");
    if (!t) {
        fprintf(stderr, "no trace");
        return 1;
    }
    ts = NULL;
    while (trace_next(t)) {
        ts = trace_get(t);
        fprintf(stderr, "%c%c %x %d\n", ts->op, ts->mem, ts->addr, ts->size);
        if (ts->mem == 'D') {
            if (ts->op == 'W') {
                cache->ops->write(cache, ts->addr, ts->size, ts->value);
            } else if (ts->op == 'R') {
                cache->ops->read(cache, ts->addr, ts->size, ts->value);
            }
        } else if (ts->op == 'I') {
            if (ts->op == 'W') {
                mem->ops->write(mem, ts->addr, ts->size, ts->value);
            } else if (ts->op == 'R') {
                mem->ops->read(mem, ts->addr, ts->size, ts->value);
            }
        }
    }
    statistics_print(info, stderr);
    
    return 0;
}
