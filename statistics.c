#include "statistics.h"

#include <stdlib.h>

void
statistics_add_counter(StatisticsInfo *info, int clock_counter)
{
    fprintf(stderr, "counter + %d\n", clock_counter);
    info->clock_counter += clock_counter;
}

void
statistics_add_hit_counter(StatisticsInfo *info)
{
    ++info->hit_counter;
}

void 
statistics_add_read(StatisticsInfo *info)
{
    ++info->read_counter;
}

void 
statistics_add_write(StatisticsInfo *info)
{
    ++info->write_counter;
}

void
statistics_add_write_back_counter(StatisticsInfo *info)
{
    ++info->write_back_counter;
}

StatisticsInfo *
statistics_free(StatisticsInfo *info)
{
    if (info != NULL) {
        free(info);
    }
    return NULL;
}

StatisticsInfo *
statistics_create(ConfigFile *cfg)
{
    if (!cfg) {
        return NULL;
    }
    StatisticsInfo *info = (StatisticsInfo *) calloc(1, sizeof(*info));
    // как быть с выбором стратегии тут?
    
    return info;
}

void 
statistics_print(StatisticsInfo *info, FILE *f)
{
    if (!info) {
        return;
    }
    FILE *out;
    if (!f) {
        out = stdout;
    } else {
        out = f;
    }
    fprintf(out, "clock count: %d\nreads: %d\nwrites: %d\n", 
        info->clock_counter, info->read_counter, info->write_counter);
    if (info->hit_counter_needed) {
        fprintf(out, "read hits: %d\n", info->hit_counter);
    }
    if (info->write_back_needed) {
        fprintf(out, "cache block writes: %d\n", info->write_back_counter);
    }
}
