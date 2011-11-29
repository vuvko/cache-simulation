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
    int lfu_count;
    int prev_idx;
    int next_idx;
} FullCacheBlock;

struct FullCache;
typedef struct FullCache FullCache;

typedef struct FullCacheOps
{
    void (*link_elem)(
        FullCache *c, 
        int idx, 
        int *first, 
        int *last);
    int (*get_used)(FullCache *c);
    void (*finalize)(FullCache *c, FullCacheBlock *b);
} FullCacheOps;

struct FullCache
{
    // Default structure
    AbstractMemory b;
    // Additional functions
    FullCacheOps full_ops;
    // Cache memory blocks
    FullCacheBlock *blocks;
    // Memory structure (as default structure)
    AbstractMemory *mem;
    Random *r;
    // LFU strategy
    int lfu_count_size;
    int lfu_max_count;
    int lfu_init_value;
    int lfu_aging_interval;
    int lfu_aging_shift;
    // List indices
    int free_first;
    int free_last;
    int used_first;
    int used_last;
    // Other cache parameters
    int cache_size; // cache size (in bytes)
    int block_size; // size of one cache block (in bytes)
    int block_count; // number of cache blocks
    int cache_read_time; // time, needed to read from cache memory
    int cache_write_time; // time, needed to write to cache memory
};

static void
full_cache_unlink_elem(
    FullCache *c, 
    int idx,
    int *first,
    int *last)
{
    if (c->blocks[idx].prev_idx < 0) {
        *first = c->blocks[idx].next_idx;
    } else {
        c->blocks[c->blocks[idx].prev_idx].next_idx = 
            c->blocks[idx].next_idx;
    }
    if (c->blocks[idx].next_idx < 0) {
        *last = c->blocks[idx].prev_idx;
    } else {
        c->blocks[c->blocks[idx].next_idx].prev_idx = 
            c->blocks[idx].prev_idx;
    }
    c->blocks[idx].next_idx = -1;
    c->blocks[idx].prev_idx = -1;
}

static void
full_cache_link_elem_first(
    FullCache *c,
    int idx,
    int *first,
    int *last)
{
    c->blocks[idx].next_idx = *first;
    if (*first >= 0) {
        c->blocks[*first].prev_idx = idx;
    }
    *first = idx;
    if (*last < 0) {
        *last = idx;
    }
}

static void
full_cache_aging(FullCache *c)
{
    int i;
    for (i = c->used_first; i >= 0; i = c->blocks[i].next_idx) {
        c->blocks[i].lfu_count >>= c->lfu_aging_shift;
    }
}

static void
full_cache_link_elem_rnd(
    FullCache *c,
    int idx,
    int *first,
    int *last)
{
    full_cache_link_elem_first(c, idx, first, last);
}

static void
full_cache_link_elem_lfu(
    FullCache *c,
    int idx,
    int *first,
    int *last)
{
    if (c->b.info->read_counter % c->lfu_aging_interval) {
        full_cache_aging(c);
    }
    if (c->blocks[idx].lfu_count < 0) {
        c->blocks[idx].lfu_count = c->lfu_init_value;
    } else if (c->blocks[idx].lfu_count < c->lfu_max_count) {
        ++c->blocks[idx].lfu_count;
    }
    if (*first < 0) {
        *first = idx;
        *last = idx;
        return;
    }
    int i;
    for (i = *first; i >= 0 &&
        c->blocks[i].lfu_count > c->blocks[idx].lfu_count;
        i = c->blocks[i].next_idx) {}
    if (i < 0) {
        c->blocks[idx].prev_idx = *last;
        c->blocks[*last].next_idx = idx;
        *last = idx;
    } else if (i == *first) {
        c->blocks[idx].next_idx = *first;
        c->blocks[*first].prev_idx = idx;
        *first = idx;
    } else {
        c->blocks[idx].next_idx = i;
        c->blocks[idx].prev_idx = c->blocks[i].prev_idx;
        c->blocks[c->blocks[i].prev_idx].next_idx = idx;
        c->blocks[i].prev_idx = idx;
    }
}

static void
full_cache_link_elem_lru(
    FullCache *c,
    int idx,
    int *first,
    int *last)
{
    full_cache_link_elem_first(c, idx, first, last);
}

int
full_cache_get_used_rnd(FullCache *c)
{
    return c->r->ops->next(c->r, c->block_count);
}

int
full_cache_get_used_lfu(FullCache *c)
{
    if (c->b.info->read_counter % c->lfu_aging_interval) {
        full_cache_aging(c);
    }
    int idx, cnt, least = c->blocks[c->used_last].lfu_count;
    for (cnt = 0, idx = c->used_last; idx >= 0 && 
        c->blocks[idx].lfu_count == least; 
        cnt++, idx = c->blocks[idx].prev_idx) {}
    cnt = c->r->ops->next(c->r, cnt);
    for (idx = c->used_last; cnt > 0; 
        cnt--, idx = c->blocks[idx].prev_idx) {}
    return idx;
}

int
full_cache_get_used_lru(FullCache *c)
{
    return c->used_last;
}

static AbstractMemory *
full_cache_free(AbstractMemory *m)
{
    if (m) {
        FullCache *c = (FullCache *) m;
        int i;
        for (i = 0; i < c->block_count; i++) {
            c->full_ops.finalize(c, &c->blocks[i]);
            free(c->blocks[i].mem);
            c->blocks[i].mem = NULL;
        }
        free(c->blocks);
        c->blocks = NULL;
        free(c);
    }
    return NULL;
}

static FullCacheBlock *
full_cache_find(FullCache *c, memaddr_t aligned_addr)
{
    int index;
    for (index = c->used_first; index >= 0 && 
         c->blocks[index].addr != aligned_addr; 
         index = c->blocks[index].next_idx) {}
    if (index < 0) {
        return NULL;
    }
    if (index != c->used_first) {
        // unlink elem
        full_cache_unlink_elem(c, index, &c->used_first, &c->used_last);
        // link elem
        c->full_ops.link_elem(c, index, &c->used_first, &c->used_last);
    }
    return &c->blocks[index];
}

static FullCacheBlock *
full_cache_place(FullCache *c, memaddr_t aligned_addr)
{
    FullCacheBlock *b;
    int index;
    if (c->free_first >= 0) {
        index = c->free_first;
        b = &c->blocks[index];
        full_cache_unlink_elem(c, index, &c->free_first, &c->free_last);
        c->full_ops.link_elem(c, index, &c->used_first, &c->used_last);
        return b;
    }
    index = c->full_ops.get_used(c);
    if (index != c->used_first) {
        full_cache_unlink_elem(c, index, &c->used_first, &c->used_last);
        c->full_ops.link_elem(c, index, &c->used_first, &c->used_last);
    }
    b = &c->blocks[index];
    if (b->addr != -1) {
        c->full_ops.finalize(c, b);
        b->addr = -1;
        memset(b->mem, 0, c->block_size * sizeof(b->mem[0]));
    }
    return b;
}

static void
full_cache_read(
    AbstractMemory *m, 
    memaddr_t addr, 
    int size, 
    MemoryCell *dst)
{
    FullCache *c = (FullCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    statistics_add_counter(c->b.info, c->cache_read_time);
    statistics_add_read(c->b.info);
    c->mem->ops->reveal(c->mem, addr, size, dst);
    FullCacheBlock *b = full_cache_find(c, aligned_addr);
    if (!b) {
        
        b = full_cache_place(c, aligned_addr);
        b->addr = aligned_addr;
        c->mem->ops->read(c->mem, aligned_addr, c->block_size, b->mem);
    } else {
        statistics_add_hit_counter(c->b.info);
    }
    memcpy(dst, b->mem + (addr - aligned_addr), 
        size * sizeof(b->mem[0]));
}

static void
full_cache_wt_write(
    AbstractMemory *m, 
    memaddr_t addr, 
    int size, 
    const MemoryCell *src)
{
    FullCache *c = (FullCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    statistics_add_write(c->b.info);
    FullCacheBlock *b = full_cache_find(c, aligned_addr);
    if (b) {
        statistics_add_counter(c->b.info, c->cache_write_time);
        memcpy(b->mem + (addr - aligned_addr), src, 
            size * sizeof(b->mem[0]));
    }
    c->mem->ops->write(c->mem, addr, size, src);
}

static void
full_cache_wb_write(
    AbstractMemory *m, 
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
    memcpy(b->mem + (addr - aligned_addr), src, 
        size * sizeof(b->mem[0]));
    b->dirty = 1;
}

static void
full_cache_reveal(
    AbstractMemory *m, 
    memaddr_t addr, 
    int size, 
    const MemoryCell *src)
{
    FullCache *c = (FullCache*) m;
    memaddr_t aligned_addr = addr & -c->block_size;
    FullCacheBlock *b = full_cache_find(c, aligned_addr);
    if (b) {
        memcpy(b->mem + (addr - aligned_addr), src, 
            size * sizeof(b->mem[0]));
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
    if (b->dirty) {
        c->mem->ops->write(c->mem, b->addr, c->block_size, b->mem);
        b->dirty = 0;
        statistics_add_write_back_counter(c->b.info);
    }
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
full_cache_create(
    ConfigFile *cfg, 
    const char *var_prefix, 
    StatisticsInfo *info, 
    AbstractMemory *mem, 
    Random *rnd)
{
    char buf[1024];
    FullCache *c = (FullCache*) calloc(1, sizeof(*c));
    c->b.info = info;
    c->b.info->hit_counter_needed = 1;
    c->r = rnd;
    srand(c->r->seed);
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
    int r, i;
    
    // стратегии замещения:
    const char *replace = config_get(cfg, make_param_name(buf, 
                    sizeof(buf), var_prefix, "replacement_strategy"));
    if (!replace) {
        error_undefined("full_cache_create", buf);
    } else if (!strcmp(replace, "random")) {
        c->full_ops.link_elem = &full_cache_link_elem_rnd;
        c->full_ops.get_used = &full_cache_get_used_rnd;
    } else if (!strcmp(replace, "lfu")) {
        c->full_ops.link_elem = &full_cache_link_elem_lfu;
        c->full_ops.get_used = &full_cache_get_used_lfu;
        r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "lfu_count_size"), &c->lfu_count_size);
        if (!r) {
            error_undefined("full_cache_create", buf);
        } else if (r < 0 || c->lfu_count_size <= 0) {
            error_invalid("full_cache_create", buf);
        }
        r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "lfu_init_value"), &c->lfu_init_value);
        if (!r) {
            error_undefined("full_cache_create", buf);
        } else if (r < 0 || c->lfu_init_value < 0) {
            error_invalid("full_cache_create", buf);
        }
        r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "lfu_aging_interval"), &c->lfu_aging_interval);
        if (!r) {
            error_undefined("full_cache_create", buf);
        } else if (r < 0 || c->lfu_aging_interval <= 0) {
            error_invalid("full_cache_create", buf);
        }
        r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
            var_prefix, "lfu_aging_shift"), &c->lfu_aging_shift);
        if (!r) {
            error_undefined("full_cache_create", buf);
        } else if (r < 0 || c->lfu_aging_shift <= 0) {
            error_invalid("full_cache_create", buf);
        }
        
        for (i = 0; i < c->lfu_count_size; i++) {
            ++c->lfu_max_count;
            c->lfu_max_count <<= 1;
        }
    } else if (!strcmp(replace, "lru")) {
        c->full_ops.link_elem = &full_cache_link_elem_lru;
        c->full_ops.get_used = &full_cache_get_used_lru;
    }

    r = config_get_int(cfg, make_param_name(buf, sizeof(buf),
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

    for (i = 0; i < c->block_count; i++) {
        c->blocks[i].mem = (MemoryCell *) calloc(c->block_size, 
                                      sizeof(c->blocks[i].mem[0]));
        c->blocks[i].addr = -1;
        c->blocks[i].lfu_count = -1;
        c->blocks[i].next_idx = i + 1;
        c->blocks[i].prev_idx = i - 1;
    }
    c->blocks[c->block_count - 1].next_idx = -1;
    c->used_first = -1;
    c->used_last = -1;
    c->free_first = 0;
    c->free_last = c->block_count - 1;

    return (AbstractMemory*) c;
}

