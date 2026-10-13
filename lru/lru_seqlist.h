#ifndef LRU_SEQLIST_H
#define LRU_SEQLIST_H

#include <stddef.h>

#include "../ds/seqlist.h"
#include "pair.h"

/*
 * 方案 A：基于动态顺序表的 LRU 缓存模拟器。
 * 顺序不变量：cache[0] 为最近刚访问项（MRU），cache[length - 1] 为最久未访问项（LRU）。
 */
typedef struct {
    SeqList* cache;           /* 底层顺序表，存储 PairType */
    size_t cache_size;        /* 缓存最大容量 k */
    size_t hit_count;         /* GET 命中次数 */
    size_t missed_count;      /* GET 未命中次数 */
    size_t access_count;      /* GET 总访问次数 */
    size_t replacement_count; /* 缓存满容时的淘汰次数 */
} LRUSeqList;

/* 初始化与销毁容量为 cache_size 的顺序表 LRU 缓存 */
int lru_seqlist_init(LRUSeqList** out, size_t cache_size);
int lru_seqlist_destroy(LRUSeqList* list);
/* 查询键 key：命中时将元素移至 cache[0] 并写出 value（返回 1），未命中将 *ret_value 置为 -1（返回 0） */
int lru_seqlist_get(LRUSeqList* list, PairElemType* key,
                    PairElemType* ret_value);
/* 插入或更新键值对，并将其置于 cache[0]；若满容且插入新键则先淘汰表尾元素 */
int lru_seqlist_put(LRUSeqList* list, PairType* pair);
/* 按从新到旧打印缓存状态与统计信息 */
int lru_seqlist_state(const LRUSeqList* list);
int lru_seqlist_stats(const LRUSeqList* list);

#endif
