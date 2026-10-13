#ifndef LRU_LINKEDLIST_H
#define LRU_LINKEDLIST_H

#include <stddef.h>

#include "../ds/linkedlist.h"
#include "pair.h"

typedef struct {
    LinkedList* cache;
    size_t cache_size;
    size_t hit_count;
    size_t missed_count;
    size_t access_count;
    size_t replacement_count;
} LRULinkedList;

int lru_linkedlist_init(LRULinkedList** out, size_t cache_size);
int lru_linkedlist_destroy(LRULinkedList* list);
int lru_linkedlist_get(LRULinkedList* list, PairElemType* key,
                       PairElemType* ret_value);
int lru_linkedlist_put(LRULinkedList* list, PairType* pair);
int lru_linkedlist_state(const LRULinkedList* list);
int lru_linkedlist_stats(const LRULinkedList* list);

#endif
