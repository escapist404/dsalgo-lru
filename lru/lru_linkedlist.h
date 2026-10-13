#ifndef LRU_LINKEDLIST_H
#define LRU_LINKEDLIST_H

#include <stddef.h>

#include "../ds/linkedlist.h"
#include "pair.h"

/*
 * 方案 B：基于带头结点单链表的 LRU 缓存模拟器。
 * 顺序不变量：head->next 为最近刚访问项（MRU），链表尾结点为最久未访问项（LRU）。
 */
typedef struct {
    LinkedList* cache;        /* 底层单链表，存储 PairType */
    size_t cache_size;        /* 缓存最大容量 k */
    size_t hit_count;         /* GET 命中次数 */
    size_t missed_count;      /* GET 未命中次数 */
    size_t access_count;      /* GET 总访问次数 */
    size_t replacement_count; /* 缓存满容时的淘汰次数 */
} LRULinkedList;

/* 初始化与销毁容量为 cache_size 的单链表 LRU 缓存 */
int lru_linkedlist_init(LRULinkedList** out, size_t cache_size);
int lru_linkedlist_destroy(LRULinkedList* list);
/* 查询键 key：命中时通过指针重链将结点移至 head 之后（返回 1），未命中返回 0 */
int lru_linkedlist_get(LRULinkedList* list, PairElemType* key,
                       PairElemType* ret_value);
/* 插入或更新键值对，并置于链表头部；满容淘汰需 O(k) 遍历定位尾结点前驱 */
int lru_linkedlist_put(LRULinkedList* list, PairType* pair);
/* 按从新到旧打印缓存状态与统计信息 */
int lru_linkedlist_state(const LRULinkedList* list);
int lru_linkedlist_stats(const LRULinkedList* list);

#endif
