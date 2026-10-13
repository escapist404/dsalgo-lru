#ifndef PAGE_REPLACEMENT_H
#define PAGE_REPLACEMENT_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PAGE_FIFO,
    PAGE_LRU,
    PAGE_LFU,
    PAGE_RANDOM,
    PAGE_MRU
} PagePolicy;

typedef struct PageCache PageCache;

int page_cache_init(PageCache **out, size_t capacity, PagePolicy policy,
                    uint32_t seed);
void page_cache_destroy(PageCache *cache);

/* Returns 1 on a hit and 0 on a miss. A miss inserts the requested page. */
int page_cache_access(PageCache *cache, int page);

size_t page_cache_hits(const PageCache *cache);
size_t page_cache_misses(const PageCache *cache);
size_t page_cache_accesses(const PageCache *cache);
const char *page_policy_name(PagePolicy policy);

#endif
