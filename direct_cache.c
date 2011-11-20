#include "direct_cache.h"

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
    if (c->blocks[index].addr != aligned_addr) return NULL;
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
direct_cache_read(AbstractMemory *m, memaddr_t addr, int size, MemoryCell *dst)
{
    DirectCache *c = (DirectCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    // FIXME: реализовать до конца
}

static void
direct_cache_wt_write(AbstractMemory *m, memaddr_t addr, int size, const MemoryCell *src)
{
    DirectCache *c = (DirectCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    statistics_add_counter(c->b.info, c->cache_write_time);
    DirectCacheBlock *b = direct_cache_find(c, aligned_addr);
    // FIXME: реализовать до конца
}

static void
direct_cache_wb_write(AbstractMemory *m, memaddr_t addr, int size, const MemoryCell *src)
{
    DirectCache *c = (DirectCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    statistics_add_counter(c->b.info, c->cache_write_time);
    DirectCacheBlock *b = direct_cache_find(c, aligned_addr);
    if (!b) {
        b = direct_cache_place(c, aligned_addr);
        b->addr = aligned_addr;
        c->mem->ops->read(c->mem, aligned_addr, c->block_size, b->mem);
    }
    memcpy(b->mem + (addr - aligned_addr), src, size * sizeof(b->mem[0]));
    b->dirty = 1;
}

static void
direct_cache_reveal(AbstractMemory *m, memaddr_t addr, int size, const MemoryCell *src)
{
    DirectCache *c = (DirectCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    DirectCacheBlock *b = direct_cache_find(c, aligned_addr);
    if (b) {
        memcpy(b->mem + (addr - aligned_addr), src, size * sizeof(b->mem[0]));
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
direct_cache_create(ConfigFile *cfg, const char *var_prefix, StatisticsInfo *info, AbstractMemory *mem, Random *rnd)
{
    char buf[1024];
    DirectCache *c = (DirectCache*) calloc(1, sizeof(*c));
    c->b.info = info;
    const char *strategy = config_file_get(cfg, make_param_name(buf, sizeof(buf), var_prefix, "write_strategy"));
    if (!strategy) {
        error_undefined("direct_cache_create", buf);
    } else if (!strcmp(strategy, "write-through")) {
        c->b.ops = &direct_cache_wt_ops;
        c->direct_ops.finalize = direct_cache_wt_finalize;
    } else if (!strcmp(strategy, "write-back")) {
        c->b.ops = &direct_cache_wb_ops;
        c->direct_ops.finalize = direct_cache_wb_finalize;
    } else {
        error_invalid("direct_cache_create", buf);
    }
    c->mem = mem;

    // FIXME: реализовать до конца

    return (AbstractMemory*) c;
}

