#ifndef LRU_HASHLIST_H
#define LRU_HASHLIST_H

#include <stddef.h>

#include "../ds/bilinkedlist.h"
#include "../ds/hashtable.h"
#include "pair.h"

typedef struct {
    HashTable* table;
    BiLinkedList* cache;
    size_t cache_size;
    size_t hit_count;
    size_t missed_count;
    size_t access_count;
    size_t replacement_count;
} LRUHashList;

int lru_hashlist_init(LRUHashList** out, size_t cache_size);
int lru_hashlist_destroy(LRUHashList* list);
int lru_hashlist_get(LRUHashList* list, PairElemType* key,
                     PairElemType* ret_value);
int lru_hashlist_put(LRUHashList* list, PairType* pair);
int lru_hashlist_state(const LRUHashList* list);
int lru_hashlist_stats(const LRUHashList* list);

#endif
