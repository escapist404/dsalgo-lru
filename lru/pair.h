#ifndef PAIR_H
#define PAIR_H

/* LRU 缓存条目键值类型定义 */
typedef int PairElemType;

/* 缓存中存储的键值对结构 */
typedef struct {
    PairElemType key;
    PairElemType value;
} PairType;

#endif
