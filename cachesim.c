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
                return 1;
                break;
        }
    }
    ConfigFile *cfg = config_read(argv[argc - 1]);
    if (!cfg) {
        return 1;
    }
    if (print_cfg) {
        config_print(cfg, NULL);
        cfg = config_free(cfg);
        return 0;
    }
    // создание структуры, хранящую статистику
    StatisticsInfo *info = statistics_create(cfg);
    if (!info) {
        return 1;
    }
    // создание нижележащей памяти
    AbstractMemory *mem = memory_create(cfg, "", info);
    if (!mem) {
        return 1;
    }
    // создание структуры генератора псевдослучайных чисел
    Random *r = random_create(cfg);
    if (!r) {
        return 1;
    }
    // создание кеша (если нужно)
    AbstractMemory *cache;
    if (en_cache) {
        cache = cache_create(cfg, "", info, mem, r);
        if (!cache) {
            return 1;
        }
    } else {
        cache = mem;
    }
    // если задан лог-файл - открываем его
    FILE *log_f = NULL;
    if (log_i > 0) {
        log_f = fopen(argv[log_i], "w");
        if (!log_f) {
            fprintf(stderr, "Can't open log file.\n");
        }
    }
    // создаём структуру, отвечающую за трассу
    Trace *t = trace_open(NULL, log_f);
    if (!t) {
        return 1;
    }
    TraceStep *ts = NULL;
    while (trace_next(t) > 0) {
        // считываем строку трассы
        ts = trace_get(t);
        // актулизируем значение кеша и памяти
        cache->ops->reveal(cache, ts->addr, ts->size, ts->value);
        if (ts->mem == 'D') {
            // происходит обращение в данным
            if (ts->op == 'W') {
                // запись данных
                statistics_add_write(info);
                cache->ops->write(cache, ts->addr, ts->size, ts->value);
            } else if (ts->op == 'R') {
                // чтение данных
                statistics_add_read(info);
                cache->ops->read(cache, ts->addr, ts->size, ts->value);
            }
        } else if (ts->mem == 'I') {
            // происходит обращение к инструкциям
            if (ts->op == 'W') {
                // запись данных
                statistics_add_write(info);
                cache->ops->write(cache, ts->addr, ts->size, ts->value);
            } else if (ts->op == 'R') {
                // чтение данных
                statistics_add_read(info);
                cache->ops->read(cache, ts->addr, ts->size, ts->value);
            }
        }
    }
    // останавливаем кеш
    if (en_cache) {
        cache = cache->ops->free(cache);
    } else {
        cache = NULL;
    }
    // вывод дампа памяти
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
    // вывод статистики
    if (stat) {
        statistics_print(info, log_f);
    }
    // освобождение ресурсов
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
