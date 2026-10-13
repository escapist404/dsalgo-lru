/* Array-based simulator for FIFO, LRU, LFU, Random, and MRU policies. */
#include <stdlib.h>
#include <string.h>

#include "page_replacement.h"

typedef struct {
    int page;
    size_t entered_at;
    size_t last_used_at;
    size_t frequency;
} PageEntry;

struct PageCache {
    PageEntry *entries;
    size_t capacity;
    size_t length;
    size_t clock;
    size_t hits;
    size_t misses;
    PagePolicy policy;
    uint32_t random_state;
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

static size_t find_page(const PageCache *cache, int page) {
    for (size_t i = 0; i < cache->length; ++i) {
        if (cache->entries[i].page == page) {
            return i;
        }
    }
    return SIZE_MAX;
}

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
    if (index != SIZE_MAX) {
        PageEntry *entry = &cache->entries[index];
        entry->last_used_at = cache->clock;
        ++entry->frequency;
        ++cache->hits;
        return 1;
    }

    ++cache->misses;
    if (cache->length == cache->capacity) {
        size_t victim = choose_victim(cache);
        memmove(&cache->entries[victim], &cache->entries[victim + 1],
                (cache->length - victim - 1) * sizeof(*cache->entries));
        --cache->length;
    }

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
