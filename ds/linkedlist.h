#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stddef.h>

/* 单链表结点定义 */
typedef struct LinkedListNode {
    void* value;                 /* 指向堆上分配的元素副本 */
    struct LinkedListNode* next; /* 后继结点指针 */
} LinkedListNode;

/* 带虚拟头结点（Dummy Head）的泛型单链表 */
typedef struct {
    LinkedListNode* head; /* 虚拟头结点，head->next 为首元结点 */
    size_t elem_size;     /* 单个元素的字节大小 */
    size_t length;        /* 当前有效元素个数 */
} LinkedList;

/* 元素匹配函数指针：匹配返回非 0，不匹配返回 0 */
typedef int (*LinkedListEqualFunc)(const void* element, const void* target);

/* 初始化与销毁单链表 */
int linkedlist_init(LinkedList** out, size_t elem_size);
void linkedlist_destroy(LinkedList* list);
size_t linkedlist_length(const LinkedList* list);
int linkedlist_empty(const LinkedList* list);

/* 结点遍历与数据访问辅助接口 */
LinkedListNode* linkedlist_head(LinkedList* list);
LinkedListNode* linkedlist_next(const LinkedListNode* node);
void* linkedlist_value(LinkedListNode* node);
const void* linkedlist_value_const(const LinkedListNode* node);

/*
 * 查找首个满足 equal(node->value, target) 的结点的【前驱结点】。
 * 返回前驱结点使得后续执行 erase_after 或 move_after 可在 O(1) 时间内完成。
 */
LinkedListNode* linkedlist_find_previous(const LinkedList* list,
                                         const void* target,
                                         LinkedListEqualFunc equal);
/* 在头结点之后插入新元素，O(1) */
int linkedlist_push_front(LinkedList* list, const void* value);
/* 删除 previous 之后的结点，O(1) */
int linkedlist_erase_after(LinkedList* list, LinkedListNode* previous,
                           void* out);
/* 遍历定位尾结点前驱并删除尾结点，O(n) */
int linkedlist_pop_back(LinkedList* list, void* out);
/* 将 previous->next 结点摘下并移动到 destination 结点之后，O(1) */
int linkedlist_move_after(LinkedList* list, LinkedListNode* previous,
                          LinkedListNode* destination);

#endif
