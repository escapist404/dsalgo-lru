/* 带首尾哨兵结点的泛型双向链表实现：支撑哈希 LRU 的 O(1) 顺序更新与尾部淘汰 */
#include <stdlib.h>
#include <string.h>

#include "bilinkedlist.h"

int bilinkedlist_init(BiLinkedList** out, size_t elem_size) {
    if (out == NULL || elem_size == 0) {
        return 0;
    }
    *out = NULL;

    BiLinkedList* list = (BiLinkedList*)calloc(1, sizeof(*list));
    if (list == NULL) {
        return 0;
    }
    /* 创建 head 与 tail 两个哨兵结点并互连，消除空表与首尾边界的特判分支 */
    list->head = (BiLinkedListNode*)calloc(1, sizeof(*list->head));
    list->tail = (BiLinkedListNode*)calloc(1, sizeof(*list->tail));
    if (list->head == NULL || list->tail == NULL) {
        free(list->head);
        free(list->tail);
        free(list);
        return 0;
    }
    list->head->next = list->tail;
    list->tail->prev = list->head;
    list->elem_size = elem_size;
    *out = list;
    return 1;
}

void bilinkedlist_destroy(BiLinkedList* list) {
    if (list == NULL) {
        return;
    }
    BiLinkedListNode* node = list->head;
    while (node != NULL) {
        BiLinkedListNode* next = node->next;
        free(node->data);
        free(node);
        node = next;
    }
    free(list);
}

size_t bilinkedlist_length(const BiLinkedList* list) {
    return list == NULL ? 0 : list->length;
}

int bilinkedlist_empty(const BiLinkedList* list) {
    return list == NULL || list->length == 0;
}

BiLinkedListNode* bilinkedlist_front(BiLinkedList* list) {
    return list == NULL || list->head->next == list->tail
               ? NULL
               : list->head->next;
}

BiLinkedListNode* bilinkedlist_back(BiLinkedList* list) {
    return list == NULL || list->tail->prev == list->head
               ? NULL
               : list->tail->prev;
}

BiLinkedListNode* bilinkedlist_next(const BiLinkedList* list,
                                   const BiLinkedListNode* node) {
    if (list == NULL || node == NULL || node->next == list->tail) {
        return NULL;
    }
    return node->next;
}

void* bilinkedlist_value(BiLinkedListNode* node) {
    return node == NULL ? NULL : node->data;
}

const void* bilinkedlist_value_const(const BiLinkedListNode* node) {
    return node == NULL ? NULL : node->data;
}

BiLinkedListNode* bilinkedlist_push_front(BiLinkedList* list,
                                          const void* value) {
    if (list == NULL || value == NULL) {
        return NULL;
    }
    BiLinkedListNode* node = (BiLinkedListNode*)calloc(1, sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    node->data = malloc(list->elem_size);
    if (node->data == NULL) {
        free(node);
        return NULL;
    }
    memcpy(node->data, value, list->elem_size);
    /* 将新结点插入到 head 与原首元结点 head->next 之间 */
    node->prev = list->head;
    node->next = list->head->next;
    list->head->next->prev = node;
    list->head->next = node;
    ++list->length;
    return node;
}

void bilinkedlist_move_to_front(BiLinkedList* list, BiLinkedListNode* node) {
    if (list == NULL || node == NULL || node == list->head ||
        node == list->tail || node == list->head->next) {
        return;
    }
    /* 先将 node 从当前位置解链，再重新挂接到 head 之后 */
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = list->head;
    node->next = list->head->next;
    list->head->next->prev = node;
    list->head->next = node;
}

void bilinkedlist_erase(BiLinkedList* list, BiLinkedListNode* node) {
    if (list == NULL || node == NULL || node == list->head ||
        node == list->tail) {
        return;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    free(node->data);
    free(node);
    --list->length;
}
