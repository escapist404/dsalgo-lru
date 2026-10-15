/* 方案 A：基于泛型顺序表的 LRU 缓存实现 */
#include <stdio.h>
#include <stdlib.h>

#include "lru_seqlist.h"

/* 按缓存条目的 key 字段判定相等性 */
static int pair_key_equal(const void* element, const void* key) {
    return ((const PairType*)element)->key == *(const PairElemType*)key;
}

int lru_seqlist_init(LRUSeqList** out, size_t cache_size) {
    if (out == NULL) {
        return 0;
    }
    *out = NULL;

    LRUSeqList* list = (LRUSeqList*)malloc(sizeof(*list));
    if (list == NULL || !seqlist_init(&list->cache, sizeof(PairType), cache_size)) {
        free(list);
        return 0;
    }

    list->cache_size = cache_size;
    list->hit_count = 0;
    list->missed_count = 0;
    list->access_count = 0;
    list->replacement_count = 0;
    *out = list;
    return 1;
}

int lru_seqlist_destroy(LRUSeqList* list) {
    if (list == NULL) {
        return 0;
    }
    seqlist_destroy(list->cache);
    free(list);
    return 1;
}

int lru_seqlist_get(LRUSeqList* list, PairElemType* key,
                    PairElemType* ret_value) {
    if (list == NULL || list->cache == NULL || key == NULL) {
        return 0;
    }

    ++list->access_count;
    size_t index = seqlist_find(list->cache, key, pair_key_equal);
    if (index == SIZE_MAX) {
        ++list->missed_count;
        if (ret_value != NULL) {
            *ret_value = -1;
        }
        return 0;
    }

    PairType found;
    if (!seqlist_get(list->cache, index, &found)) {
        ++list->missed_count;
        return 0;
    }
    if (ret_value != NULL) {
        *ret_value = found.value;
    }
    ++list->hit_count;

    /* 命中后将 [0, index - 1] 区间的元素依次后移一格，并将命中元素置于表头下标 0 */
    for (size_t i = index; i != 0; --i) {
        PairType previous;
        seqlist_get(list->cache, i - 1, &previous);
        seqlist_set(list->cache, i, &previous);
    }
    if (index != 0) {
        seqlist_set(list->cache, 0, &found);
    }
    return 1;
}

int lru_seqlist_put(LRUSeqList* list, PairType* pair) {
    if (list == NULL || list->cache == NULL || pair == NULL ||
        list->cache_size == 0) {
        return 0;
    }

    PairType copy = *pair;
    size_t index = seqlist_find(list->cache, &copy.key, pair_key_equal);
    /* 若键已存在，先删除旧位置条目（不计入淘汰次数），随后统一在表头插入最新值 */
    if (index != SIZE_MAX && !seqlist_erase(list->cache, index, NULL)) {
        return 0;
    }
    /* 若键不存在且缓存已满，驱逐表尾最旧元素 */
    if (seqlist_length(list->cache) == list->cache_size) {
        if (!seqlist_pop_back(list->cache, NULL)) {
            return 0;
        }
        ++list->replacement_count;
    }
    return seqlist_insert(list->cache, 0, &copy);
}

int lru_seqlist_state(const LRUSeqList* list) {
    if (list == NULL || list->cache == NULL) {
        return 0;
    }
    if (seqlist_empty(list->cache)) {
        puts("EMPTY");
        return 1;
    }
    printf("STATE");
    for (size_t i = 0; i < seqlist_length(list->cache); ++i) {
        const PairType* pair = (const PairType*)seqlist_at_const(list->cache, i);
        printf(" %d:%d", pair->key, pair->value);
    }
    puts("");
    return 1;
}

int lru_seqlist_stats(const LRUSeqList* list) {
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
