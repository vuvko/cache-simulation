#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trace.h"
#include "common.h"
#include "memory.h"
#include "parsecfg.h"
#include "statistics.h"
#include "cache.h"
#include "random.h"
#include "abstract_memory.h"

int
main(int argc, char *argv[])
{
    int i, idx, cfg_i = 0, log_i = 0, dmp = 0, stat = 0, en_cache = 1;
    char *params[] = {
        "--disable-cache",
        "--dump-memory",
        "--log",
        "--statistics"
    };
    for (i = 1; i < argc; i++) {
        if ((idx = arrayidx_str(argv[i], params, 
            sizeof(params) / sizeof(params[0]))) > 0) {
            switch (idx) {
                case 0:
                    en_cache = 0;
                    break;
                case 1:
                    dmp = 1;
                    break;
                case 2:
                    log_i = ++i;
                    break;
                default:
                    stat = 1;
                    break;
            }
        } else if (!cfg_i) {
            cfg_i = i;
        } else {
            fprintf(stderr, "Invalid parameter: %s\n", argv[i]);
            return 1;
        }
    }
    fprintf(stderr, "Config\n");
    ConfigFile *cfg = config_read(argv[cfg_i]);
    if (!cfg) {
        fprintf(stderr, "no cfg\n");
        return 1;
    }
    StatisticsInfo *info = statistics_create(cfg);
    if (!info) {
        fprintf(stderr, "no info\n");
        return 1;
    }
    AbstractMemory *mem = memory_create(cfg, "", info);
    if (!mem) {
        fprintf(stderr, "no mem\n");
        return 1;
    }
    Random *r = random_create(cfg);
    if (!r) {
        fprintf(stderr, "no random\n");
        return 1;
    }
    AbstractMemory *cache;
    if (en_cache) {
        cache = cache_create(cfg, "", info, mem, r);
        if (!cache) {
            fprintf(stderr, "no cache\n");
            return 1;
        }
    } else {
        cache = mem;
    }
    //---
    Trace *t = trace_open("trace.fire", NULL);
    if (!t) {
        fprintf(stderr, "no trace");
        return 1;
    }
    TraceStep *ts = NULL;
    /*
    while (trace_next(t)) {
        ts = trace_get(t);
        if (ts->mem == 'D') {
            cache->ops->reveal(cache, ts->addr, ts->size, ts->value);
        } else if (ts->op == 'I') {
            mem->ops->reveal(mem, ts->addr, ts->size, ts->value);
        }
    }*/
    t = trace_close(t);
    //---
    FILE *log_f = NULL;
    if (log_i > 0) {
        log_f = fopen(argv[log_i], "w");
        if (!log_f) {
            fprintf(stderr, "Can't open log file.\n");
        }
    }
    t = trace_open("trace.trc", log_f);
    if (!t) {
        fprintf(stderr, "no trace");
        return 1;
    }
    ts = NULL;
    while (trace_next(t) > 0) {
        ts = trace_get(t);
        fprintf(stderr, "|%c%c %x %d\n", ts->op, ts->mem, ts->addr, ts->size);
        if (ts->mem == 'D') {
            if (ts->op == 'W') {
                //mem->ops->write(mem, ts->addr, ts->size, ts->value);
                cache->ops->write(cache, ts->addr, ts->size, ts->value);
            } else if (ts->op == 'R') {
                //mem->ops->read(mem, ts->addr, ts->size, ts->value);
                cache->ops->read(cache, ts->addr, ts->size, ts->value);
            }
        } else if (ts->mem == 'I') {
            if (ts->op == 'W') {
                //mem->ops->write(mem, ts->addr, ts->size, ts->value);
                cache->ops->write(cache, ts->addr, ts->size, ts->value);
            } else if (ts->op == 'R') {
                //mem->ops->read(mem, ts->addr, ts->size, ts->value);
                cache->ops->read(cache, ts->addr, ts->size, ts->value);
            }
        }
    }
    if (stat) {
        statistics_print(info, stderr);
    }
    if (dmp) {
        fprintf(stderr, "ALIVE!!!\n");
        FILE *dmp = fopen("dupm.dmp", "w");
        fprintf(stderr, "ALIVE!!!\n");
        mem_dump(mem, dmp);
        fprintf(stderr, "ALIVE!!!\n");
        fclose(dmp);
        fprintf(stderr, "ALIVE!!!\n");
    }
    
    t = trace_close(t);
    fprintf(stderr, "ALIVE!!!\n");
    cache = cache->ops->free(cache);
    fprintf(stderr, "ALIVE!!!\n");
    info = statistics_free(info);
    fprintf(stderr, "ALIVE!!!\n");
    cfg = config_free(cfg);
    
    if (log_f) {
        fclose(log_f);
    }
    
    return 0;
}
