#ifndef SEQLIST_H
#define SEQLIST_H

#include <stddef.h>

/* 元素相等性比较函数指针：相等返回非 0，不等返回 0 */
typedef int (*SeqListEqualFunc)(const void* a, const void* b);

/* 泛型动态顺序表（连续内存存储） */
typedef struct SeqList SeqList;

/* 初始化顺序表，指定单个元素字节大小 elem_size 与初始容量 initial_capacity */
int seqlist_init(SeqList** out, size_t elem_size, size_t initial_capacity);
/* 销毁顺序表并释放底层连续内存 */
void seqlist_destroy(SeqList* list);

/* 判断顺序表是否为空 */
int seqlist_empty(const SeqList* list);
/* 清空顺序表（仅将长度置 0，保留已分配容量） */
void seqlist_clear(SeqList* list);

/* 获取当前元素个数与底层数组容量 */
size_t seqlist_length(const SeqList* list);
size_t seqlist_capacity(const SeqList* list);

/* 按下标返回元素的可变/只读地址（越界返回 NULL） */
void* seqlist_at(SeqList* list, size_t index);
const void* seqlist_at_const(const SeqList* list, size_t index);
/* 按下标读取元素到 out，或将 elem 写入指定下标位置 */
int seqlist_get(const SeqList* list, size_t index, void* out);
int seqlist_set(SeqList* list, size_t index, const void* elem);

/* 预留至少 new_capacity 个元素的存储容量 */
int seqlist_reserve(SeqList* list, size_t new_capacity);
/* 在指定下标处插入元素，后续元素整体后移 O(n) */
int seqlist_insert(SeqList* list, size_t index, const void* elem);
/* 在表尾追加元素，均摊 O(1) */
int seqlist_push_back(SeqList* list, const void* elem);
/* 删除指定下标处的元素（可选通过 out 拷出被删元素），后续元素整体前移 O(n) */
int seqlist_erase(SeqList* list, size_t index, void* out);
/* 删除表尾元素，O(1) */
int seqlist_pop_back(SeqList* list, void* out);

/* 顺序查找首个满足 equal(elem, target) 的元素下标，未找到返回 SIZE_MAX */
size_t seqlist_find(const SeqList* list, const void* target,
                    SeqListEqualFunc equal);

#endif
