#include <stdlib.h>
#include "memory.h"

enum
{
    MAX_MEM_SIZE = 1 * GiB,
    MAX_READ_TIME = 100000,
    MAX_WRITE_TIME = MAX_READ_TIME,
    MAX_WIDTH = 1024
};

typedef struct Memory
{
    AbstractMemory b;
    MemoryCell *mem;
    int memory_size;
    int memory_read_time;
    int memory_write_time;
    int memory_width;
} Memory;

AbstractMemory *
memory_free(AbstractMemory *a)
{
    if (a != NULL) {
        Memory *m = (Memory *) a;
        free(m->mem);
        free(m);
    }
    return NULL;
}

void
memory_read(AbstractMemory *a, memaddr_t addr, int size, MemoryCell *dst)
{
    Memory *m = (Memory*) a;
fprintf(stderr, "|r|%d %d\n", (size + m->memory_width - 1) / 
                           m->memory_width, size);
    statistics_add_counter(m->b.info, (size + m->memory_width - 1) / 
                           m->memory_width * m->memory_read_time);
    statistics_add_read(m->b.info);

    for (; size; ++addr, --size, ++dst) {
        *dst = m->mem[addr];
    }
}

void
memory_write(AbstractMemory *a, memaddr_t addr, int size, const MemoryCell *src)
{
    Memory *m = (Memory *) a;
fprintf(stderr, "|w|%d %d\n", (size + m->memory_width - 1) / 
                           m->memory_width, size);
    statistics_add_counter(m->b.info, (size + m->memory_width - 1) /
                           m->memory_width * m->memory_write_time);
    statistics_add_write(m->b.info);
    
    for (; size; ++addr, --size, ++src) {
        m->mem[addr] = *src;
    }
}

static void
memory_reveal(AbstractMemory *a, memaddr_t addr, int size, const MemoryCell *src)
{
    Memory *m = (Memory *) a;

    for (; size; ++addr, --size, ++src) {
        m->mem[addr] = *src;
    }
}

static AbstractMemoryOps memory_ops =
{
    memory_free,
    memory_read,
    memory_write,
    memory_reveal
};

AbstractMemory *
memory_create(ConfigFile *cfg, const char *var_prefix, StatisticsInfo *info)
{
    if (!info || !var_prefix || !cfg) {
        return NULL;
    }
    char buf[1024];
    Memory *m = calloc(1, sizeof(*m));
    int r;

    m->b.ops = &memory_ops;
    m->b.info = info;

    r = config_get_int(cfg, make_param_name(buf, sizeof(buf), 
                       var_prefix, "memory_size"), &m->memory_size);
    if (!r) {
        error_undefined("memory_create", buf);
    } else if (r < 0 || m->memory_size <= 0 || 
               m->memory_size > MAX_MEM_SIZE || 
               m->memory_size % KiB != 0) {
        error_invalid("memory_create", buf);
    }
    r = config_get_int(cfg, make_param_name(buf, sizeof(buf), 
            var_prefix, "memory_read_time"), &m->memory_read_time);
    if (!r) {
        error_undefined("memory_create", buf);
    } else if (r < 0 || m->memory_read_time < 0 || 
               m->memory_read_time > MAX_READ_TIME) {
        error_invalid("memory_create", buf);
    }
    r = config_get_int(cfg, make_param_name(buf, sizeof(buf), 
            var_prefix, "memory_write_time"), &m->memory_write_time);
    if (!r) {
        error_undefined("memory_create", buf);
    } else if (r < 0 || m->memory_write_time < 0 || 
               m->memory_write_time > MAX_WRITE_TIME) {
        error_invalid("memory_create", buf);
    }
    r = config_get_int(cfg, make_param_name(buf, sizeof(buf), 
                       var_prefix, "memory_width"), &m->memory_width);
    if (!r) {
        error_undefined("memory_create", buf);
    } else if (r < 0 || m->memory_width <= 0 || 
               m->memory_width > MAX_WIDTH) {
        error_invalid("memory_create", buf);
    }

    m->mem = (MemoryCell *) calloc(m->memory_size, sizeof(m->mem[0]));

    return (AbstractMemory *) m;
}
