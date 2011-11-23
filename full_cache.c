#include "full_cache.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>

enum
{
    MAX_CACHE_SIZE = 16 * MiB,
    MAX_READ_TIME = 100000,
    MAX_WRITE_TIME = MAX_READ_TIME
};

typedef struct FullCacheBlock
{
    memaddr_t addr;
    MemoryCell *mem;
    int dirty;
} FullCacheBlock;

struct FullCache;
typedef struct FullCache FullCache;

typedef struct FullCacheOps
{
    void (*finalize)(FullCache *c, FullCacheBlock *b);
} FullCacheOps;

struct FullCache
{
    AbstractMemory b;
    FullCacheOps full_ops;
    FullCacheBlock *blocks;
    AbstractMemory *mem;
    int cache_size;
    int block_size;
    int block_count;
    int cache_read_time;
    int cache_write_time;
};

static AbstractMemory *
full_cache_free(AbstractMemory *m)
{
    if (m) {
        FullCache *c = (FullCache *) m;
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

static FullCacheBlock *
full_cache_find(FullCache *c, memaddr_t aligned_addr)
{
    // FIXME: rewrite!
    int index = (aligned_addr / c->block_size) % c->block_count;
    if (c->blocks[index].addr != aligned_addr) {
        return NULL;
    }
    return &c->blocks[index];
}

static FullCacheBlock *
full_cache_place(FullCache *c, memaddr_t aligned_addr)
{
    // FIXME: rewrite!
    int index = (aligned_addr / c->block_size) % c->block_count;
    FullCacheBlock *b = &c->blocks[index];
    if (b->addr != -1) {
        c->full_ops.finalize(c, b);
        b->addr = -1;
        memset(b->mem, 0, c->block_size * sizeof(b->mem[0]));
    }
    return b;
}

static void
full_cache_read(AbstractMemory *m, 
                  memaddr_t addr, 
                  int size, 
                  MemoryCell *dst)
{
    FullCache *c = (FullCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    statistics_add_counter(c->b.info, c->cache_read_time);
    statistics_add_read(c->b.info);
    FullCacheBlock *b = full_cache_find(c, aligned_addr);
    if (!b) {
        b = full_cache_place(c, aligned_addr);
        b->addr = aligned_addr;
        c->mem->ops->read(c->mem, aligned_addr, c->block_size, b->mem);
    } else {
        statistics_add_hit_counter(c->b.info);
    }
    memcpy(dst, b->mem + (addr - aligned_addr), size * sizeof(b->mem[0]));
}

static void
full_cache_wt_write(AbstractMemory *m, 
                      memaddr_t addr, 
                      int size, 
                      const MemoryCell *src)
{
    FullCache *c = (FullCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    statistics_add_counter(c->b.info, c->cache_write_time);
    statistics_add_write(c->b.info);
    FullCacheBlock *b = full_cache_find(c, aligned_addr);
    if (!b) {
        b = full_cache_place(c, aligned_addr);
        b->addr = aligned_addr;
        c->mem->ops->read(c->mem, aligned_addr, c->block_size, b->mem);
    }
    memcpy(b->mem + (addr - aligned_addr), src, size * sizeof(b->mem[0]));
    c->mem->ops->write(c->mem, addr, size, src);
}

static void
full_cache_wb_write(AbstractMemory *m, 
                      memaddr_t addr, 
                      int size, 
                      const MemoryCell *src)
{
    FullCache *c = (FullCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    statistics_add_counter(c->b.info, c->cache_write_time);
    statistics_add_write(c->b.info);
    FullCacheBlock *b = full_cache_find(c, aligned_addr);
    if (!b) {
        b = full_cache_place(c, aligned_addr);
        b->addr = aligned_addr;
        c->mem->ops->read(c->mem, aligned_addr, c->block_size, b->mem);
    }
    memcpy(b->mem + (addr - aligned_addr), src, size * sizeof(b->mem[0]));
    b->dirty = 1;
}

static void
full_cache_reveal(AbstractMemory *m, 
                    memaddr_t addr, 
                    int size, 
                    const MemoryCell *src)
{
    FullCache *c = (FullCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    FullCacheBlock *b = full_cache_find(c, aligned_addr);
    if (b) {
        memcpy(b->mem + (addr - aligned_addr), src, size * sizeof(b->mem[0]));
    }
    c->mem->ops->reveal(c->mem, addr, size, src);
}

static void
full_cache_wt_finalize(FullCache *c, FullCacheBlock *b)
{
    // ничего не делаем...
}

static void
full_cache_wb_finalize(FullCache *c, FullCacheBlock *b)
{
    // записываем грязный блок в память...
    c->mem->ops->write(c->mem, b->addr, c->block_size, b->mem);
    b->dirty = 0;
}

static AbstractMemoryOps full_cache_wt_ops =
{
    full_cache_free,
    full_cache_read,
    full_cache_wt_write,
    full_cache_reveal,
};

static AbstractMemoryOps full_cache_wb_ops =
{
    full_cache_free,
    full_cache_read,
    full_cache_wb_write,
    full_cache_reveal,
};

AbstractMemory *
full_cache_create(ConfigFile *cfg, 
                    const char *var_prefix, 
                    StatisticsInfo *info, 
                    AbstractMemory *mem, 
                    Random *rnd)
{
    char buf[1024];
    FullCache *c = (FullCache*) calloc(1, sizeof(*c));
    c->b.info = info;
    c->b.info->hit_counter_needed = 1;
    const char *strategy = config_get(cfg, make_param_name(buf, 
                           sizeof(buf), var_prefix, "write_strategy"));
    if (!strategy) {
        error_undefined("full_cache_create", buf);
    } else if (!strcmp(strategy, "write-through")) {
        c->b.ops = &full_cache_wt_ops;
        c->full_ops.finalize = full_cache_wt_finalize;
    } else if (!strcmp(strategy, "write-back")) {
        c->b.ops = &full_cache_wb_ops;
        c->full_ops.finalize = full_cache_wb_finalize;
        c->b.info->write_back_needed = 1;
    } else {
        error_invalid("full_cache_create", buf);
    }
    c->mem = mem;
    
    int r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "cache_size"), &c->cache_size);
    if (!r) {
        error_undefined("full_cache_create", buf);
    } else if (r < 0 || c->cache_size > MAX_CACHE_SIZE ||
               c->cache_size <= 0) {
        error_invalid("full_cache_create", buf);
    }
    r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "block_size"), &c->block_size);
    if (!r) {
        error_undefined("full_cache_create", buf);
    } else if (r < 0 || c->block_size > c->cache_size || 
               c->block_size <= 0 ||
               c->cache_size % c->block_size != 0) {
        error_invalid("full_cache_create", buf);
    }
    r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "cache_read_time"), &c->cache_read_time);
    if (!r) {
        error_undefined("full_cache_create", buf);
    } else if (r < 0 || c->cache_read_time < 0 ||
               c->cache_read_time > MAX_READ_TIME) {
        error_invalid("full_cache_create", buf);
    }
    r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "cache_write_time"), &c->cache_write_time);
    if (!r) {
        error_undefined("full_cache_create", buf);
    } else if (r < 0 || c->cache_write_time < 0 ||
               c->cache_write_time > MAX_WRITE_TIME) {
        error_invalid("full_cache_create", buf);
    }
    c->block_count = c->cache_size / c->block_size;
    c->blocks = (FullCacheBlock *) calloc(c->block_count, 
                                            sizeof(c->blocks[0]));
    int i;
    for (i = 0; i < c->block_count; i++) {
        c->blocks[i].mem = (MemoryCell *) calloc(c->block_size, 
                                      sizeof(c->blocks[i].mem[0]));
        c->blocks[i].addr = -1;
    }

    return (AbstractMemory*) c;
}

