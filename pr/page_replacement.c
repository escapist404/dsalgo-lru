/* 多策略页面置换模拟器实现：统一模拟 FIFO、LRU、LFU、Random 与 MRU */
#include <stdlib.h>
#include <string.h>

#include "page_replacement.h"

/* 单个驻留页面的元数据条目 */
typedef struct {
    int page;            /* 页面编号 */
    size_t entered_at;   /* 进入缓存时的逻辑时间戳（用于 FIFO） */
    size_t last_used_at; /* 最近一次被访问的逻辑时间戳（用于 LRU / MRU / LFU 平局） */
    size_t frequency;    /* 当前驻留周期内的访问频次（用于 LFU） */
} PageEntry;

struct PageCache {
    PageEntry *entries;    /* 驻留页面数组 */
    size_t capacity;       /* 缓存容量上限 */
    size_t length;         /* 当前驻留页面数 */
    size_t clock;          /* 单调递增逻辑时钟（每次访问自增 1） */
    size_t hits;           /* 累计命中次数 */
    size_t misses;         /* 累计未命中次数 */
    PagePolicy policy;     /* 当前启用的置换策略 */
    uint32_t random_state; /* Random 策略专用的伪随机数状态 */
};

static uint32_t next_random(uint32_t *state) {
    if (*state == 0) {
        *state = 1;
    }
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

/* 在驻留页面数组中线性查找目标页面下标，未找到返回 SIZE_MAX */
static size_t find_page(const PageCache *cache, int page) {
    for (size_t i = 0; i < cache->length; ++i) {
        if (cache->entries[i].page == page) {
            return i;
        }
    }
    return SIZE_MAX;
}

/* 根据当前置换策略选择需要驱逐的牺牲页（victim）下标 */
static size_t choose_victim(PageCache *cache) {
    size_t victim = 0;
    for (size_t i = 1; i < cache->length; ++i) {
        const PageEntry *candidate = &cache->entries[i];
        const PageEntry *current = &cache->entries[victim];
        int replace = 0;
        if (cache->policy == PAGE_FIFO) {
            replace = candidate->entered_at < current->entered_at;
        } else if (cache->policy == PAGE_LRU) {
            replace = candidate->last_used_at < current->last_used_at;
        } else if (cache->policy == PAGE_LFU) {
            /* LFU 优先淘汰访问频次最少者；若频次相同则按 LRU 淘汰最久未访问者 */
            replace = candidate->frequency < current->frequency ||
                      (candidate->frequency == current->frequency &&
                       candidate->last_used_at < current->last_used_at);
        } else if (cache->policy == PAGE_MRU) {
            replace = candidate->last_used_at > current->last_used_at;
        }
        if (replace) {
            victim = i;
        }
    }
    if (cache->policy == PAGE_RANDOM) {
        victim = next_random(&cache->random_state) % cache->length;
    }
    return victim;
}

int page_cache_init(PageCache **out, size_t capacity, PagePolicy policy,
                    uint32_t seed) {
    if (out == NULL || capacity == 0 || policy < PAGE_FIFO ||
        policy > PAGE_MRU) {
        return 0;
    }
    *out = NULL;
    PageCache *cache = (PageCache *)calloc(1, sizeof(*cache));
    if (cache == NULL) {
        return 0;
    }
    cache->entries = (PageEntry *)calloc(capacity, sizeof(*cache->entries));
    if (cache->entries == NULL) {
        free(cache);
        return 0;
    }
    cache->capacity = capacity;
    cache->policy = policy;
    cache->random_state = seed == 0 ? 1 : seed;
    *out = cache;
    return 1;
}

void page_cache_destroy(PageCache *cache) {
    if (cache == NULL) {
        return;
    }
    free(cache->entries);
    free(cache);
}

int page_cache_access(PageCache *cache, int page) {
    if (cache == NULL) {
        return 0;
    }
    ++cache->clock;
    size_t index = find_page(cache, page);
    /* 页面命中：刷新最近访问时间戳与驻留期访问频次 */
    if (index != SIZE_MAX) {
        PageEntry *entry = &cache->entries[index];
        entry->last_used_at = cache->clock;
        ++entry->frequency;
        ++cache->hits;
        return 1;
    }

    /* 页面未命中：若缓存已满则先根据策略驱逐一个牺牲页 */
    ++cache->misses;
    if (cache->length == cache->capacity) {
        size_t victim = choose_victim(cache);
        memmove(&cache->entries[victim], &cache->entries[victim + 1],
                (cache->length - victim - 1) * sizeof(*cache->entries));
        --cache->length;
    }

    /* 装入新页面并初始化其时间戳与访问频次 */
    PageEntry entry = {
        .page = page,
        .entered_at = cache->clock,
        .last_used_at = cache->clock,
        .frequency = 1,
    };
    cache->entries[cache->length++] = entry;
    return 0;
}

size_t page_cache_hits(const PageCache *cache) {
    return cache == NULL ? 0 : cache->hits;
}

size_t page_cache_misses(const PageCache *cache) {
    return cache == NULL ? 0 : cache->misses;
}

size_t page_cache_accesses(const PageCache *cache) {
    return cache == NULL ? 0 : cache->hits + cache->misses;
}

const char *page_policy_name(PagePolicy policy) {
    static const char *const names[] = {
        "FIFO", "LRU", "LFU", "Random", "MRU"};
    return policy >= PAGE_FIFO && policy <= PAGE_MRU ? names[policy]
                                                    : "Unknown";
}
