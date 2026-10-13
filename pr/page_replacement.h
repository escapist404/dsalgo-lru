#ifndef PAGE_REPLACEMENT_H
#define PAGE_REPLACEMENT_H

#include <stddef.h>
#include <stdint.h>

/* 支持的五种在线页面置换策略枚举 */
typedef enum {
    PAGE_FIFO,   /* 先进先出：淘汰最早装入缓存的页面 */
    PAGE_LRU,    /* 最近最少使用：淘汰最久未被访问的页面 */
    PAGE_LFU,    /* 最不经常使用：淘汰驻留期间访问频次最少的页面（平局按 LRU） */
    PAGE_RANDOM, /* 随机置换：随机选择一个驻留页面淘汰 */
    PAGE_MRU     /* 最近最常使用：淘汰最近刚被访问过的页面 */
} PagePolicy;

/* 页面置换缓存模拟器不透明结构体 */
typedef struct PageCache PageCache;

/* 初始化页面置换模拟器（指定容量 capacity、策略 policy 与随机数种子 seed） */
int page_cache_init(PageCache **out, size_t capacity, PagePolicy policy,
                    uint32_t seed);
/* 销毁模拟器并释放内存 */
void page_cache_destroy(PageCache *cache);

/* 访问指定页面 page：命中返回 1 并更新元数据；未命中返回 0 并在必要时淘汰旧页后装入新页 */
int page_cache_access(PageCache *cache, int page);

/* 获取累计命中次数、未命中次数、总访问次数及策略名称 */
size_t page_cache_hits(const PageCache *cache);
size_t page_cache_misses(const PageCache *cache);
size_t page_cache_accesses(const PageCache *cache);
const char *page_policy_name(PagePolicy policy);

#endif
