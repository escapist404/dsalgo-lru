/* 方案 C：基于哈希表与双向链表组合的 O(1) LRU 缓存实现 */
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
    /* 哈希表的值直接保存双向链表结点指针 BiLinkedListNode* */
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
    /* 通过哈希表在平均 O(1) 时间内定位双向链表结点 */
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
    /* O(1) 将命中结点移动至双向链表头部 */
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
    /* 若键已存在，原地更新结点值并移至链表头部 */
    if (found != NULL) {
        PairType* stored = (PairType*)bilinkedlist_value(*found);
        *stored = copy;
        bilinkedlist_move_to_front(list->cache, *found);
        return 1;
    }

    /* 缓存已满时，从双向链表尾部取最旧结点，同步从哈希表与链表中移除 */
    if (bilinkedlist_length(list->cache) == list->cache_size) {
        BiLinkedListNode* old = bilinkedlist_back(list->cache);
        PairType* old_pair = (PairType*)bilinkedlist_value(old);
        hashtable_remove(list->table, &old_pair->key);
        bilinkedlist_erase(list->cache, old);
        ++list->replacement_count;
    }

    /* 在链表头部插入新结点，并将其结点指针注册到哈希表中 */
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
    if (bilinkedlist_empty(list->cache)) {
        puts("EMPTY");
        return 1;
    }
    printf("STATE");
    for (BiLinkedListNode* node = bilinkedlist_front(list->cache);
         node != NULL; node = bilinkedlist_next(list->cache, node)) {
        const PairType* pair =
            (const PairType*)bilinkedlist_value_const(node);
        printf(" %d:%d", pair->key, pair->value);
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
    printf("STATS %zu %zu %zu %.6f\n",
           list->hit_count, list->missed_count, list->replacement_count,
           hit_rate);
    return 1;
}
