#ifndef LRU_HASHLIST_H
#define LRU_HASHLIST_H

#include <stddef.h>

#include "../ds/bilinkedlist.h"
#include "../ds/hashtable.h"
#include "pair.h"

/*
 * 方案 C：工业界标准的“哈希表 + 双向链表”LRU 缓存模拟器。
 * table 维护 key -> BiLinkedListNode* 的平均 O(1) 索引；
 * cache 维护从新（head->next）到旧（tail->prev）的访问顺序，支持 O(1) 提升与尾部淘汰。
 */
typedef struct {
    HashTable* table;         /* 键到双向链表结点指针的映射表 */
    BiLinkedList* cache;      /* 维护新旧顺序的双向链表 */
    size_t cache_size;        /* 缓存最大容量 k */
    size_t hit_count;         /* GET 命中次数 */
    size_t missed_count;      /* GET 未命中次数 */
    size_t access_count;      /* GET 总访问次数 */
    size_t replacement_count; /* 缓存满容时的淘汰次数 */
} LRUHashList;

/* 初始化与销毁哈希+双向链表 LRU 缓存 */
int lru_hashlist_init(LRUHashList** out, size_t cache_size);
int lru_hashlist_destroy(LRUHashList* list);
/* 平均 O(1) 查询键 key 并提升至双向链表头部 */
int lru_hashlist_get(LRUHashList* list, PairElemType* key,
                     PairElemType* ret_value);
/* 平均 O(1) 插入或更新键值对，满容时 O(1) 驱逐双向链表尾结点并同步删除哈希表项 */
int lru_hashlist_put(LRUHashList* list, PairType* pair);
/* 按从新到旧打印缓存状态与统计信息 */
int lru_hashlist_state(const LRUHashList* list);
int lru_hashlist_stats(const LRUHashList* list);

#endif
