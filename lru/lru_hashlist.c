/* LRU policy combining hash lookup with doubly linked recency order. */
#include <stdio.h>
#include <stdlib.h>

#include "lru_hashlist.h"

int lru_hashlist_init(LRUHashList** out, size_t cache_size) {
    if (out == NULL || cache_size == 0) {
        return 0;
    }
    *out = NULL;

    LRUHashList* list = (LRUHashList*)calloc(1, sizeof(*list));
    if (list == NULL) {
        return 0;
    }
    if (!hashtable_init(&list->table, sizeof(PairElemType),
                        sizeof(BiLinkedListNode*), hashtable_int_hash,
                        hashtable_int_equal) ||
        !bilinkedlist_init(&list->cache, sizeof(PairType))) {
        hashtable_destroy(list->table);
        bilinkedlist_destroy(list->cache);
        free(list);
        return 0;
    }
    list->cache_size = cache_size;
    *out = list;
    return 1;
}

int lru_hashlist_destroy(LRUHashList* list) {
    if (list == NULL) {
        return 0;
    }
    hashtable_destroy(list->table);
    bilinkedlist_destroy(list->cache);
    free(list);
    return 1;
}

int lru_hashlist_get(LRUHashList* list, PairElemType* key,
                     PairElemType* ret_value) {
    if (list == NULL || list->table == NULL || list->cache == NULL ||
        key == NULL) {
        return 0;
    }

    ++list->access_count;
    BiLinkedListNode** found =
        (BiLinkedListNode**)hashtable_get(list->table, key);
    if (found == NULL) {
        ++list->missed_count;
        if (ret_value != NULL) {
            *ret_value = -1;
        }
        return 0;
    }

    PairType* pair = (PairType*)bilinkedlist_value(*found);
    if (ret_value != NULL) {
        *ret_value = pair->value;
    }
    ++list->hit_count;
    bilinkedlist_move_to_front(list->cache, *found);
    return 1;
}

int lru_hashlist_put(LRUHashList* list, PairType* pair) {
    if (list == NULL || list->table == NULL || list->cache == NULL ||
        pair == NULL || list->cache_size == 0) {
        return 0;
    }

    PairType copy = *pair;
    BiLinkedListNode** found =
        (BiLinkedListNode**)hashtable_get(list->table, &copy.key);
    if (found != NULL) {
        PairType* stored = (PairType*)bilinkedlist_value(*found);
        *stored = copy;
        bilinkedlist_move_to_front(list->cache, *found);
        return 1;
    }

    if (bilinkedlist_length(list->cache) == list->cache_size) {
        BiLinkedListNode* old = bilinkedlist_back(list->cache);
        PairType* old_pair = (PairType*)bilinkedlist_value(old);
        hashtable_remove(list->table, &old_pair->key);
        bilinkedlist_erase(list->cache, old);
        ++list->replacement_count;
    }

    BiLinkedListNode* node = bilinkedlist_push_front(list->cache, &copy);
    if (node == NULL) {
        return 0;
    }
    if (!hashtable_put(list->table, &copy.key, &node)) {
        bilinkedlist_erase(list->cache, node);
        return 0;
    }
    return 1;
}

int lru_hashlist_state(const LRUHashList* list) {
    if (list == NULL || list->cache == NULL) {
        return 0;
    }
    printf("STATE");
    for (BiLinkedListNode* node = bilinkedlist_front(list->cache);
         node != NULL; node = bilinkedlist_next(list->cache, node)) {
        const PairType* pair =
            (const PairType*)bilinkedlist_value_const(node);
        printf(" %d:%d", pair->key, pair->value);
    }
    if (bilinkedlist_empty(list->cache)) {
        printf(" EMPTY");
    }
    puts("");
    return 1;
}

int lru_hashlist_stats(const LRUHashList* list) {
    if (list == NULL) {
        return 0;
    }
    double hit_rate = list->access_count == 0
                          ? 0.0
                          : (double)list->hit_count / list->access_count;
    printf("STATS hits=%zu misses=%zu replacements=%zu hit_rate=%.6f\n",
           list->hit_count, list->missed_count, list->replacement_count,
           hit_rate);
    return 1;
}
