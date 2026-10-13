#ifndef SEQLIST_H
#define SEQLIST_H

#include <stddef.h>

typedef int (*SeqListEqualFunc)(const void* a, const void* b);

typedef struct SeqList SeqList;

int seqlist_init(SeqList** out, size_t elem_size, size_t initial_capacity);
void seqlist_destroy(SeqList* list);

int seqlist_empty(const SeqList* list);
void seqlist_clear(SeqList* list);

size_t seqlist_length(const SeqList* list);
size_t seqlist_capacity(const SeqList* list);

void* seqlist_at(SeqList* list, size_t index);
const void* seqlist_at_const(const SeqList* list, size_t index);
int seqlist_get(const SeqList* list, size_t index, void* out);
int seqlist_set(SeqList* list, size_t index, const void* elem);

int seqlist_reserve(SeqList* list, size_t new_capacity);
int seqlist_insert(SeqList* list, size_t index, const void* elem);
int seqlist_push_back(SeqList* list, const void* elem);
int seqlist_erase(SeqList* list, size_t index, void* out);
int seqlist_pop_back(SeqList* list, void* out);

size_t seqlist_find(const SeqList* list, const void* target,
                    SeqListEqualFunc equal);

#endif
