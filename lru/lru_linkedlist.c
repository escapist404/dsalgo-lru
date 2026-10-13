/* 方案 B：基于泛型单链表的 LRU 缓存实现 */
#include <stdio.h>
#include <stdlib.h>

#include "lru_linkedlist.h"

static int pair_key_equal(const void* element, const void* key) {
    return ((const PairType*)element)->key == *(const PairElemType*)key;
}

int lru_linkedlist_init(LRULinkedList** out, size_t cache_size) {
    if (out == NULL) {
        return 0;
    }
    *out = NULL;

    LRULinkedList* list = (LRULinkedList*)calloc(1, sizeof(*list));
    if (list == NULL ||
        !linkedlist_init(&list->cache, sizeof(PairType))) {
        free(list);
        return 0;
    }
    list->cache_size = cache_size;
    *out = list;
    return 1;
}

int lru_linkedlist_destroy(LRULinkedList* list) {
    if (list == NULL) {
        return 0;
    }
    linkedlist_destroy(list->cache);
    free(list);
    return 1;
}

int lru_linkedlist_get(LRULinkedList* list, PairElemType* key,
                       PairElemType* ret_value) {
    if (list == NULL || list->cache == NULL || key == NULL) {
        return 0;
    }

    ++list->access_count;
    /* 查找目标结点的前驱，使命中后的表头提升可在 O(1) 内通过修改指针完成 */
    LinkedListNode* previous =
        linkedlist_find_previous(list->cache, key, pair_key_equal);
    if (previous == NULL) {
        ++list->missed_count;
        if (ret_value != NULL) {
            *ret_value = -1;
        }
        return 0;
    }

    LinkedListNode* node = linkedlist_next(previous);
    const PairType* found =
        (const PairType*)linkedlist_value_const(node);
    if (ret_value != NULL) {
        *ret_value = found->value;
    }
    ++list->hit_count;

    linkedlist_move_after(list->cache, previous,
                          linkedlist_head(list->cache));
    return 1;
}

int lru_linkedlist_put(LRULinkedList* list, PairType* pair) {
    if (list == NULL || list->cache == NULL || pair == NULL ||
        list->cache_size == 0) {
        return 0;
    }

    PairType copy = *pair;
    LinkedListNode* head = linkedlist_head(list->cache);
    LinkedListNode* previous =
        linkedlist_find_previous(list->cache, &copy.key, pair_key_equal);
    /* 若键已存在，原地覆写结点中的键值对并将其移动至 head 之后 */
    if (previous != NULL) {
        LinkedListNode* node = linkedlist_next(previous);
        PairType* stored = (PairType*)linkedlist_value(node);
        *stored = copy;
        return linkedlist_move_after(list->cache, previous, head);
    }

    /* 若缓存已满，调用 pop_back 淘汰尾部最旧结点 */
    if (linkedlist_length(list->cache) == list->cache_size) {
        if (!linkedlist_pop_back(list->cache, NULL)) {
            return 0;
        }
        ++list->replacement_count;
    }
    return linkedlist_push_front(list->cache, &copy);
}

int lru_linkedlist_state(const LRULinkedList* list) {
    if (list == NULL || list->cache == NULL) {
        return 0;
    }
    printf("STATE");
    for (LinkedListNode* node =
             linkedlist_next(linkedlist_head(list->cache));
         node != NULL; node = linkedlist_next(node)) {
        const PairType* pair =
            (const PairType*)linkedlist_value_const(node);
        printf(" %d:%d", pair->key, pair->value);
    }
    if (linkedlist_empty(list->cache)) {
        printf(" EMPTY");
    }
    puts("");
    return 1;
}

int lru_linkedlist_stats(const LRULinkedList* list) {
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
