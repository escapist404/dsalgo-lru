#ifndef BILINKEDLIST_H
#define BILINKEDLIST_H

#include <stddef.h>

/* 双向链表结点定义 */
typedef struct BiLinkedListNode {
    void* data;                    /* 指向堆上分配的元素副本 */
    struct BiLinkedListNode* prev; /* 前驱结点指针 */
    struct BiLinkedListNode* next; /* 后继结点指针 */
} BiLinkedListNode;

/*
 * 带首尾虚拟哨兵结点（head / tail）的泛型双向链表。
 * 有效元素位于 head->next 与 tail->prev 之间，所有边界插入与删除均为 O(1)。
 */
typedef struct {
    BiLinkedListNode* head; /* 头部哨兵结点 */
    BiLinkedListNode* tail; /* 尾部哨兵结点 */
    size_t elem_size;       /* 单个元素的字节大小 */
    size_t length;          /* 当前有效元素个数 */
} BiLinkedList;

/* 初始化与销毁双向链表 */
int bilinkedlist_init(BiLinkedList** out, size_t elem_size);
void bilinkedlist_destroy(BiLinkedList* list);
size_t bilinkedlist_length(const BiLinkedList* list);
int bilinkedlist_empty(const BiLinkedList* list);

/* 获取首元结点（最新）、尾元结点（最旧）及后继有效结点 */
BiLinkedListNode* bilinkedlist_front(BiLinkedList* list);
BiLinkedListNode* bilinkedlist_back(BiLinkedList* list);
BiLinkedListNode* bilinkedlist_next(const BiLinkedList* list,
                                   const BiLinkedListNode* node);
void* bilinkedlist_value(BiLinkedListNode* node);
const void* bilinkedlist_value_const(const BiLinkedListNode* node);

/* 在表头（head 之后）插入新元素并返回结点指针，O(1) */
BiLinkedListNode* bilinkedlist_push_front(BiLinkedList* list,
                                          const void* value);
/* 将链表内已知结点摘下并移动至表头（head 之后），O(1) */
void bilinkedlist_move_to_front(BiLinkedList* list, BiLinkedListNode* node);
/* 从链表中摘除并释放已知结点，O(1) */
void bilinkedlist_erase(BiLinkedList* list, BiLinkedListNode* node);

#endif
