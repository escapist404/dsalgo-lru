#ifndef BILINKEDLIST_H
#define BILINKEDLIST_H

#include <stddef.h>

typedef struct BiLinkedListNode {
    void* data;
    struct BiLinkedListNode* prev;
    struct BiLinkedListNode* next;
} BiLinkedListNode;

typedef struct {
    BiLinkedListNode* head;
    BiLinkedListNode* tail;
    size_t elem_size;
    size_t length;
} BiLinkedList;

int bilinkedlist_init(BiLinkedList** out, size_t elem_size);
void bilinkedlist_destroy(BiLinkedList* list);
size_t bilinkedlist_length(const BiLinkedList* list);
int bilinkedlist_empty(const BiLinkedList* list);
BiLinkedListNode* bilinkedlist_front(BiLinkedList* list);
BiLinkedListNode* bilinkedlist_back(BiLinkedList* list);
BiLinkedListNode* bilinkedlist_next(const BiLinkedList* list,
                                   const BiLinkedListNode* node);
void* bilinkedlist_value(BiLinkedListNode* node);
const void* bilinkedlist_value_const(const BiLinkedListNode* node);
BiLinkedListNode* bilinkedlist_push_front(BiLinkedList* list,
                                          const void* value);
void bilinkedlist_move_to_front(BiLinkedList* list, BiLinkedListNode* node);
void bilinkedlist_erase(BiLinkedList* list, BiLinkedListNode* node);

#endif
