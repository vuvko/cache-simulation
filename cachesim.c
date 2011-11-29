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
    int dmp_i = 0;
    char *params[] = {
        "--disable-cache",
        "-d",
        "--dump-memory",
        "-m",
        "--log",
        "-l",
        "--statistics",
        "-s",
        "--dump-file",
        "-mf"
    };
    for (i = 1; i < argc; i++) {
        if ((idx = arrayidx_str(argv[i], params, 
            sizeof(params) / sizeof(params[0]))) >= 0) {
            switch (idx) {
                case 0:
                case 1:
                    en_cache = 0;
                    break;
                case 2:
                case 3:
                    dmp = 1;
                    break;
                case 4:
                case 5:
                    log_i = ++i;
                    break;
                case 6:
                case 7:
                    stat = 1;
                    break;
                case 8:
                case 9:
                    dmp_i = ++i;
                    break;
                default:
                    break;
            }
        } else if (!cfg_i) {
            cfg_i = i;
        } else {
            fprintf(stderr, "Invalid parameter: %s\n", argv[i]);
            return 1;
        }
    }
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
    FILE *log_f = NULL;
    if (log_i > 0) {
        log_f = fopen(argv[log_i], "w");
        if (!log_f) {
            fprintf(stderr, "Can't open log file.\n");
        }
    }
    Trace *t = trace_open("trace.trc", log_f);
    if (!t) {
        fprintf(stderr, "no trace");
        return 1;
    }
    TraceStep *ts = NULL;
    while (trace_next(t) > 0) {;
        ts = trace_get(t);
        cache->ops->reveal(cache, ts->addr, ts->size, ts->value);
        fprintf(stderr, "%c%c %x %d %02x%02x%02x %d%d%d\n", 
            ts->op, ts->mem, ts->addr, ts->size,
            ts->value[0].value, ts->value[1].value, ts->value[2].value, 
            ts->value[0].flags, ts->value[1].flags, ts->value[2].flags);
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
    if (en_cache) {
        cache = cache->ops->free(cache);
    } else {
        cache = NULL;
    }
    if (dmp) {
        FILE *dmp;
        if (dmp_i > 0) {
            dmp = fopen(argv[dmp_i], "w");
        } else {
            dmp = stdout;
        }
        mem_dump(mem, dmp);
        if (dmp_i > 0) {
            fclose(dmp);
        }
    }
    if (stat) {
        statistics_print(info, log_f);
    }
    
    t = trace_close(t);
    mem = mem->ops->free(mem);
    r = r->ops->free(r);
    info = statistics_free(info);
    cfg = config_free(cfg);
    
    if (log_i > 0) {
        fclose(log_f);
    }
    
    return 0;
}
