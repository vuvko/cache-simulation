#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

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
    int log_i = 0, dmp = 0, stat = 0, en_cache = 1;
    int dmp_i = 0, print_cfg = 0;
    /*
    int i, idx, cfg_i = 0;
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
        "-mf",
        "--print-config",
        "-p",
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
                case 10:
                case 11:
                    print_cfg = 1;
                default:
                    break;
            }
        } else if (!cfg_i) {
            cfg_i = i;
        } else {
            fprintf(stderr, "Invalid argument: %s\n", argv[i]);
            return 1;
        }
    }
    */
    int res = 0, optidx = -1;
    const char *shortopt = "dmsp";
    const struct option longopt[] = {
        {"disable-cache", no_argument, NULL, 'd'},
        {"dump-memory", no_argument, NULL, 'm'},
        {"statistics", no_argument, NULL, 's'},
        {"print-config", no_argument, NULL, 'p'},
        {NULL, 0, NULL, 0}
    };
    
    // разбор параметров командной строки
    while ((res = getopt_long(argc, argv, shortopt, longopt, &optidx)) > 0) {
        switch (res) {
            case 'd':
                en_cache = 0;
                break;
            case 'm':
                dmp = 1;
                break;
            case 's':
                stat = 1;
                break;
            case 'p':
                print_cfg = 1;
                break;
            default:
                fprintf(stderr, "Invalid arguments\n");
                return 0;
                break;
        }
    }
    ConfigFile *cfg = config_read(argv[argc - 1]);
    if (!cfg) {
        fprintf(stderr, "no cfg\n");
        return 1;
    }
    if (print_cfg) {
        config_print(cfg, NULL);
        cfg = config_free(cfg);
        return 0;
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
        //cache->ops->reveal(cache, ts->addr, ts->size, ts->value);
        /*
        fprintf(stderr, "%c%c %x %d %02x%02x%02x %d%d%d\n", 
            ts->op, ts->mem, ts->addr, ts->size,
            ts->value[0].value, ts->value[1].value, ts->value[2].value, 
            ts->value[0].flags, ts->value[1].flags, ts->value[2].flags);*/
        if (ts->mem == 'D') {
            if (ts->op == 'W') {
                statistics_add_write(info);
                //mem->ops->write(mem, ts->addr, ts->size, ts->value);
                cache->ops->write(cache, ts->addr, ts->size, ts->value);
            } else if (ts->op == 'R') {
                statistics_add_read(info);
                //mem->ops->read(mem, ts->addr, ts->size, ts->value);
                cache->ops->read(cache, ts->addr, ts->size, ts->value);
            }
        } else if (ts->mem == 'I') {
            if (ts->op == 'W') {
                statistics_add_write(info);
                //mem->ops->write(mem, ts->addr, ts->size, ts->value);
                cache->ops->write(cache, ts->addr, ts->size, ts->value);
            } else if (ts->op == 'R') {
                statistics_add_read(info);
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
        FILE *dmp_f;
        if (dmp_i > 0) {
            dmp_f = fopen(argv[dmp_i], "w");
        } else {
            dmp_f = stdout;
        }
        mem_dump(mem, dmp_f);
        if (dmp_i > 0) {
            fclose(dmp_f);
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
