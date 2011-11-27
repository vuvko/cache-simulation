#include "direct_cache.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>

enum
{
    MAX_CACHE_SIZE = 16 * MiB,
    MAX_READ_TIME = 100000,
    MAX_WRITE_TIME = MAX_READ_TIME
};

typedef struct DirectCacheBlock
{
    memaddr_t addr;
    MemoryCell *mem;
    int dirty;
} DirectCacheBlock;

struct DirectCache;
typedef struct DirectCache DirectCache;

typedef struct DirectCacheOps
{
    void (*finalize)(DirectCache *c, DirectCacheBlock *b);
} DirectCacheOps;

struct DirectCache
{
    AbstractMemory b;
    DirectCacheOps direct_ops;
    DirectCacheBlock *blocks;
    AbstractMemory *mem;
    int cache_size;
    int block_size;
    int block_count;
    int cache_read_time;
    int cache_write_time;
};

static AbstractMemory *
direct_cache_free(AbstractMemory *m)
{
    if (m) {
        DirectCache *c = (DirectCache *) m;
        c->mem = c->mem->ops->free(c->mem);
        int i;
        for (i = 0; i < c->block_count; i++) {
            free(c->blocks[i].mem);
        }
        free(c->blocks);
        free(c);
    }
    return NULL;
}

static DirectCacheBlock *
direct_cache_find(DirectCache *c, memaddr_t aligned_addr)
{
    int index = (aligned_addr / c->block_size) % c->block_count;
    if (c->blocks[index].addr != aligned_addr) {
        return NULL;
    }
    return &c->blocks[index];
}

static DirectCacheBlock *
direct_cache_place(DirectCache *c, memaddr_t aligned_addr)
{
    int index = (aligned_addr / c->block_size) % c->block_count;
    DirectCacheBlock *b = &c->blocks[index];
    if (b->addr != -1) {
        c->direct_ops.finalize(c, b);
        b->addr = -1;
        memset(b->mem, 0, c->block_size * sizeof(b->mem[0]));
    }
    return b;
}

static void
direct_cache_read(
    AbstractMemory *m, 
    memaddr_t addr, 
    int size, 
    MemoryCell *dst)
{
    DirectCache *c = (DirectCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    statistics_add_counter(c->b.info, c->cache_read_time);
    statistics_add_read(c->b.info);
    DirectCacheBlock *b = direct_cache_find(c, aligned_addr);
    if (!b) {
        b = direct_cache_place(c, aligned_addr);
        b->addr = aligned_addr;
        c->mem->ops->read(c->mem, aligned_addr, c->block_size, b->mem);
    } else {
        statistics_add_hit_counter(c->b.info);
    }
    memcpy(dst, b->mem + (addr - aligned_addr), 
        size * sizeof(b->mem[0]));
}

static void
direct_cache_wt_write(
    AbstractMemory *m, 
    memaddr_t addr, 
    int size, 
    const MemoryCell *src)
{
    DirectCache *c = (DirectCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    statistics_add_counter(c->b.info, c->cache_write_time);
    statistics_add_write(c->b.info);
    DirectCacheBlock *b = direct_cache_find(c, aligned_addr);
    if (!b) {
        b = direct_cache_place(c, aligned_addr);
        b->addr = aligned_addr;
        c->mem->ops->read(c->mem, aligned_addr, c->block_size, b->mem);
    }
    memcpy(b->mem + (addr - aligned_addr), src, 
        size * sizeof(b->mem[0]));
    c->mem->ops->write(c->mem, addr, size, src);
}

static void
direct_cache_wb_write(
    AbstractMemory *m, 
    memaddr_t addr, 
    int size, 
    const MemoryCell *src)
{
    DirectCache *c = (DirectCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    statistics_add_counter(c->b.info, c->cache_write_time);
    statistics_add_write(c->b.info);
    DirectCacheBlock *b = direct_cache_find(c, aligned_addr);
    if (!b) {
        b = direct_cache_place(c, aligned_addr);
        b->addr = aligned_addr;
        c->mem->ops->read(c->mem, aligned_addr, c->block_size, b->mem);
    }
    memcpy(b->mem + (addr - aligned_addr), src, 
        size * sizeof(b->mem[0]));
    b->dirty = 1;
}

static void
direct_cache_reveal(
    AbstractMemory *m, 
    memaddr_t addr, 
    int size, 
    const MemoryCell *src)
{
    DirectCache *c = (DirectCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    DirectCacheBlock *b = direct_cache_find(c, aligned_addr);
    if (b) {
        memcpy(b->mem + (addr - aligned_addr), src, 
            size * sizeof(b->mem[0]));
    }
    c->mem->ops->reveal(c->mem, addr, size, src);
}

static void
direct_cache_wt_finalize(DirectCache *c, DirectCacheBlock *b)
{
    // ничего не делаем...
}

static void
direct_cache_wb_finalize(DirectCache *c, DirectCacheBlock *b)
{
    // записываем грязный блок в память...
    c->mem->ops->write(c->mem, b->addr, c->block_size, b->mem);
    b->dirty = 0;
}

static AbstractMemoryOps direct_cache_wt_ops =
{
    direct_cache_free,
    direct_cache_read,
    direct_cache_wt_write,
    direct_cache_reveal,
};

static AbstractMemoryOps direct_cache_wb_ops =
{
    direct_cache_free,
    direct_cache_read,
    direct_cache_wb_write,
    direct_cache_reveal,
};

AbstractMemory *
direct_cache_create(
    ConfigFile *cfg, 
    const char *var_prefix, 
    StatisticsInfo *info, 
    AbstractMemory *mem, 
    Random *rnd)
{
    char buf[1024];
    DirectCache *c = (DirectCache*) calloc(1, sizeof(*c));
    c->b.info = info;
    c->b.info->hit_counter_needed = 1;
    const char *strategy = config_get(cfg, make_param_name(buf, 
                           sizeof(buf), var_prefix, "write_strategy"));
    if (!strategy) {
        error_undefined("direct_cache_create", buf);
    } else if (!strcmp(strategy, "write-through")) {
        c->b.ops = &direct_cache_wt_ops;
        c->direct_ops.finalize = direct_cache_wt_finalize;
    } else if (!strcmp(strategy, "write-back")) {
        c->b.ops = &direct_cache_wb_ops;
        c->direct_ops.finalize = direct_cache_wb_finalize;
        c->b.info->write_back_needed = 1;
    } else {
        error_invalid("direct_cache_create", buf);
    }
    c->mem = mem;
    
    int r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "cache_size"), &c->cache_size);
    if (!r) {
        error_undefined("direct_cache_create", buf);
    } else if (r < 0 || c->cache_size > MAX_CACHE_SIZE ||
               c->cache_size <= 0) {
        error_invalid("direct_cache_create", buf);
    }
    r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "block_size"), &c->block_size);
    if (!r) {
        error_undefined("direct_cache_create", buf);
    } else if (r < 0 || c->block_size > c->cache_size || 
               c->block_size <= 0 ||
               c->cache_size % c->block_size != 0) {
        error_invalid("direct_cache_create", buf);
    }
    r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "cache_read_time"), &c->cache_read_time);
    if (!r) {
        error_undefined("direct_cache_create", buf);
    } else if (r < 0 || c->cache_read_time < 0 ||
               c->cache_read_time > MAX_READ_TIME) {
        error_invalid("direct_cache_create", buf);
    }
    r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "cache_write_time"), &c->cache_write_time);
    if (!r) {
        error_undefined("direct_cache_create", buf);
    } else if (r < 0 || c->cache_write_time < 0 ||
               c->cache_write_time > MAX_WRITE_TIME) {
        error_invalid("direct_cache_create", buf);
    }
    c->block_count = c->cache_size / c->block_size;
    c->blocks = (DirectCacheBlock *) calloc(c->block_count, 
                                            sizeof(c->blocks[0]));
    int i;
    for (i = 0; i < c->block_count; i++) {
        c->blocks[i].mem = (MemoryCell *) calloc(c->block_size, 
                                      sizeof(c->blocks[i].mem[0]));
        c->blocks[i].addr = -1;
    }

    return (AbstractMemory*) c;
}

