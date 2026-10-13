/* Generic dynamically sized array used by the array-backed LRU. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "seqlist.h"

struct SeqList {
    void* data;
    size_t elem_size;
    size_t length;
    size_t capacity;
};

int seqlist_init(SeqList** out, size_t elem_size, size_t initial_capacity) {
    if (out == NULL || elem_size == 0 ||
        initial_capacity > SIZE_MAX / elem_size) {
        return 0;
    }

    *out = NULL;
    SeqList* list = (SeqList*)calloc(1, sizeof(*list));
    if (list == NULL) {
        return 0;
    }
    list->elem_size = elem_size;
    if (initial_capacity != 0) {
        list->data = malloc(initial_capacity * elem_size);
        if (list->data == NULL) {
            free(list);
            return 0;
        }
        list->capacity = initial_capacity;
    }
    *out = list;
    return 1;
}

void seqlist_destroy(SeqList* list) {
    if (list == NULL) {
        return;
    }
    free(list->data);
    free(list);
}

void seqlist_clear(SeqList* list) {
    if (list != NULL) {
        list->length = 0;
    }
}

size_t seqlist_length(const SeqList* list) {
    return list == NULL ? 0 : list->length;
}

size_t seqlist_capacity(const SeqList* list) {
    return list == NULL ? 0 : list->capacity;
}

int seqlist_empty(const SeqList* list) {
    return list == NULL || list->length == 0;
}

void* seqlist_at(SeqList* list, size_t index) {
    if (list == NULL || index >= list->length) {
        return NULL;
    }
    return (unsigned char*)list->data + index * list->elem_size;
}

const void* seqlist_at_const(const SeqList* list, size_t index) {
    if (list == NULL || index >= list->length) {
        return NULL;
    }
    return (const unsigned char*)list->data + index * list->elem_size;
}

int seqlist_get(const SeqList* list, size_t index, void* out) {
    if (list == NULL || out == NULL || index >= list->length) {
        return 0;
    }
    memcpy(out, (const unsigned char*)list->data + index * list->elem_size,
           list->elem_size);
    return 1;
}

int seqlist_set(SeqList* list, size_t index, const void* elem) {
    if (list == NULL || elem == NULL || index >= list->length) {
        return 0;
    }
    memcpy((unsigned char*)list->data + index * list->elem_size, elem,
           list->elem_size);
    return 1;
}

int seqlist_reserve(SeqList* list, size_t new_capacity) {
    if (list == NULL) {
        return 0;
    }
    if (new_capacity <= list->capacity) {
        return 1;
    }
    if (new_capacity > SIZE_MAX / list->elem_size) {
        return 0;
    }
    void* data = realloc(list->data, new_capacity * list->elem_size);
    if (data == NULL) {
        return 0;
    }
    list->data = data;
    list->capacity = new_capacity;
    return 1;
}

int seqlist_insert(SeqList* list, size_t index, const void* elem) {
    if (list == NULL || elem == NULL || index > list->length) {
        return 0;
    }
    if (list->length == list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 1 : list->capacity * 2;
        if (new_capacity < list->capacity ||
            !seqlist_reserve(list, new_capacity)) {
            return 0;
        }
    }

    unsigned char* data = (unsigned char*)list->data;
    size_t offset = index * list->elem_size;
    size_t tail = (list->length - index) * list->elem_size;
    memmove(data + offset + list->elem_size, data + offset, tail);
    memmove(data + offset, elem, list->elem_size);
    ++list->length;
    return 1;
}

int seqlist_push_back(SeqList* list, const void* elem) {
    return list == NULL ? 0 : seqlist_insert(list, list->length, elem);
}

int seqlist_erase(SeqList* list, size_t index, void* out) {
    if (list == NULL || index >= list->length) {
        return 0;
    }
    unsigned char* data = (unsigned char*)list->data;
    size_t offset = index * list->elem_size;
    size_t tail = (list->length - index - 1) * list->elem_size;
    if (out != NULL) {
        memcpy(out, data + offset, list->elem_size);
    }
    memmove(data + offset, data + offset + list->elem_size, tail);
    --list->length;
    return 1;
}

int seqlist_pop_back(SeqList* list, void* out) {
    return list == NULL || list->length == 0
               ? 0
               : seqlist_erase(list, list->length - 1, out);
}

size_t seqlist_find(const SeqList* list, const void* target,
                    SeqListEqualFunc equal) {
    if (list == NULL || target == NULL || equal == NULL) {
        return SIZE_MAX;
    }
    const unsigned char* data = (const unsigned char*)list->data;
    for (size_t i = 0; i < list->length; ++i) {
        if (equal(data + i * list->elem_size, target)) {
            return i;
        }
    }
    return SIZE_MAX;
}
