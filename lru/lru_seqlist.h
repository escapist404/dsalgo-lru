#ifndef LRU_SEQLIST_H
#define LRU_SEQLIST_H

#include <stddef.h>

#include "../ds/seqlist.h"
#include "pair.h"

typedef struct {
    SeqList* cache;
    size_t cache_size;
    size_t hit_count;
    size_t missed_count;
    size_t access_count;
    size_t replacement_count;
} LRUSeqList;

int lru_seqlist_init(LRUSeqList** out, size_t cache_size);
int lru_seqlist_destroy(LRUSeqList* list);
int lru_seqlist_get(LRUSeqList* list, PairElemType* key,
                    PairElemType* ret_value);
int lru_seqlist_put(LRUSeqList* list, PairType* pair);
int lru_seqlist_state(const LRUSeqList* list);
int lru_seqlist_stats(const LRUSeqList* list);

#endif
