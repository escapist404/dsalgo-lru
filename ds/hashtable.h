#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>

#include "linkedlist.h"
#include "seqlist.h"

/* 哈希函数与键相等性比较函数指针类型 */
typedef size_t (*HashTableHashFunc)(const void* key);
typedef int (*HashTableEqualFunc)(const void* a, const void* b);

/* 哈希表冲突链中的键值对结点内容 */
typedef struct HashTableEntry {
    void* key;
    void* value;
} HashTableEntry;

/*
 * 基于链地址法（Separate Chaining）的泛型哈希表。
 * 桶数组使用 SeqList（元素类型为 LinkedList*），每个非空桶挂载一条单链表。
 */
typedef struct {
    SeqList* buckets;        /* 桶数组，存储 LinkedList* 指针 */
    size_t bucket_count;     /* 当前哈希桶数量 */
    size_t size;             /* 当前已存储的键值对总数 */
    size_t key_size;         /* 键的字节大小 */
    size_t value_size;       /* 值的字节大小 */
    HashTableHashFunc hash;  /* 键哈希函数 */
    HashTableEqualFunc equal;/* 键相等比较函数 */
} HashTable;

/* 初始化与销毁泛型哈希表 */
int hashtable_init(HashTable** out, size_t key_size, size_t value_size,
                   HashTableHashFunc hash, HashTableEqualFunc equal);
void hashtable_destroy(HashTable* table);

/* 按键查找对应值的地址（未找到返回 NULL），平均 O(1) */
void* hashtable_get(HashTable* table, const void* key);
const void* hashtable_get_const(const HashTable* table, const void* key);
/* 插入或更新键值对（装载因子超 0.75 时自动倍增扩容），平均 O(1) */
int hashtable_put(HashTable* table, const void* key, const void* value);
/* 按键删除键值对，平均 O(1) */
int hashtable_remove(HashTable* table, const void* key);
size_t hashtable_size(const HashTable* table);
int hashtable_empty(const HashTable* table);

/* 针对 int 类型键的内置位混淆哈希函数与相等比较函数 */
size_t hashtable_int_hash(const void* key);
int hashtable_int_equal(const void* a, const void* b);

#endif
