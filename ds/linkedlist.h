#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stddef.h>

typedef struct LinkedListNode {
    void* value;
    struct LinkedListNode* next;
} LinkedListNode;

typedef struct {
    LinkedListNode* head;
    size_t elem_size;
    size_t length;
} LinkedList;

typedef int (*LinkedListEqualFunc)(const void* element, const void* target);

int linkedlist_init(LinkedList** out, size_t elem_size);
void linkedlist_destroy(LinkedList* list);
size_t linkedlist_length(const LinkedList* list);
int linkedlist_empty(const LinkedList* list);

LinkedListNode* linkedlist_head(LinkedList* list);
LinkedListNode* linkedlist_next(const LinkedListNode* node);
void* linkedlist_value(LinkedListNode* node);
const void* linkedlist_value_const(const LinkedListNode* node);

LinkedListNode* linkedlist_find_previous(const LinkedList* list,
                                         const void* target,
                                         LinkedListEqualFunc equal);
int linkedlist_push_front(LinkedList* list, const void* value);
int linkedlist_erase_after(LinkedList* list, LinkedListNode* previous,
                           void* out);
int linkedlist_pop_back(LinkedList* list, void* out);
int linkedlist_move_after(LinkedList* list, LinkedListNode* previous,
                          LinkedListNode* destination);

#endif
