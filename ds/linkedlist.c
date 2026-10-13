/* 带虚拟头结点的泛型单链表实现：支持 O(1) 头插与已知前驱下的 O(1) 结点搬移 */
#include <stdlib.h>
#include <string.h>

#include "linkedlist.h"

int linkedlist_init(LinkedList** out, size_t elem_size) {
    if (out == NULL || elem_size == 0) {
        return 0;
    }
    *out = NULL;

    LinkedList* list = (LinkedList*)calloc(1, sizeof(*list));
    if (list == NULL) {
        return 0;
    }
    /* 分配不存储业务数据的虚拟头结点，统一首元结点与内部结点的指针操作 */
    list->head = (LinkedListNode*)calloc(1, sizeof(*list->head));
    if (list->head == NULL) {
        free(list);
        return 0;
    }
    list->elem_size = elem_size;
    *out = list;
    return 1;
}

void linkedlist_destroy(LinkedList* list) {
    if (list == NULL) {
        return;
    }
    LinkedListNode* node = list->head;
    while (node != NULL) {
        LinkedListNode* next = node->next;
        free(node->value);
        free(node);
        node = next;
    }
    free(list);
}

size_t linkedlist_length(const LinkedList* list) {
    return list == NULL ? 0 : list->length;
}

int linkedlist_empty(const LinkedList* list) {
    return list == NULL || list->length == 0;
}

LinkedListNode* linkedlist_head(LinkedList* list) {
    return list == NULL ? NULL : list->head;
}

LinkedListNode* linkedlist_next(const LinkedListNode* node) {
    return node == NULL ? NULL : node->next;
}

void* linkedlist_value(LinkedListNode* node) {
    return node == NULL ? NULL : node->value;
}

const void* linkedlist_value_const(const LinkedListNode* node) {
    return node == NULL ? NULL : node->value;
}

LinkedListNode* linkedlist_find_previous(const LinkedList* list,
                                         const void* target,
                                         LinkedListEqualFunc equal) {
    if (list == NULL || target == NULL || equal == NULL) {
        return NULL;
    }
    LinkedListNode* previous = list->head;
    for (LinkedListNode* node = previous->next; node != NULL;
         previous = node, node = node->next) {
        if (equal(node->value, target)) {
            return previous;
        }
    }
    return NULL;
}

int linkedlist_push_front(LinkedList* list, const void* value) {
    if (list == NULL || value == NULL) {
        return 0;
    }
    LinkedListNode* node = (LinkedListNode*)calloc(1, sizeof(*node));
    if (node == NULL) {
        return 0;
    }
    node->value = malloc(list->elem_size);
    if (node->value == NULL) {
        free(node);
        return 0;
    }
    memcpy(node->value, value, list->elem_size);
    node->next = list->head->next;
    list->head->next = node;
    ++list->length;
    return 1;
}

int linkedlist_erase_after(LinkedList* list, LinkedListNode* previous,
                           void* out) {
    if (list == NULL || previous == NULL || previous->next == NULL) {
        return 0;
    }
    LinkedListNode* removed = previous->next;
    previous->next = removed->next;
    if (out != NULL) {
        memcpy(out, removed->value, list->elem_size);
    }
    free(removed->value);
    free(removed);
    --list->length;
    return 1;
}

int linkedlist_pop_back(LinkedList* list, void* out) {
    if (list == NULL || list->head->next == NULL) {
        return 0;
    }
    /* 单链表无前驱指针，需从头遍历至倒数第二个结点 */
    LinkedListNode* previous = list->head;
    while (previous->next->next != NULL) {
        previous = previous->next;
    }
    return linkedlist_erase_after(list, previous, out);
}

int linkedlist_move_after(LinkedList* list, LinkedListNode* previous,
                          LinkedListNode* destination) {
    if (list == NULL || previous == NULL || destination == NULL ||
        previous->next == NULL || previous == destination ||
        destination == previous->next) {
        return previous == destination;
    }
    /* 原地摘下 previous->next 并重链到 destination 之后，无需重新分配结点 */
    LinkedListNode* node = previous->next;
    previous->next = node->next;
    node->next = destination->next;
    destination->next = node;
    return 1;
}
