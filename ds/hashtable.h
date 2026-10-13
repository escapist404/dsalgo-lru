#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>

#include "linkedlist.h"
#include "seqlist.h"

typedef size_t (*HashTableHashFunc)(const void* key);
typedef int (*HashTableEqualFunc)(const void* a, const void* b);

typedef struct HashTableEntry {
    void* key;
    void* value;
} HashTableEntry;

typedef struct {
    SeqList* buckets;
    size_t bucket_count;
    size_t size;
    size_t key_size;
    size_t value_size;
    HashTableHashFunc hash;
    HashTableEqualFunc equal;
} HashTable;

int hashtable_init(HashTable** out, size_t key_size, size_t value_size,
                   HashTableHashFunc hash, HashTableEqualFunc equal);
void hashtable_destroy(HashTable* table);
void* hashtable_get(HashTable* table, const void* key);
const void* hashtable_get_const(const HashTable* table, const void* key);
int hashtable_put(HashTable* table, const void* key, const void* value);
int hashtable_remove(HashTable* table, const void* key);
size_t hashtable_size(const HashTable* table);
int hashtable_empty(const HashTable* table);

size_t hashtable_int_hash(const void* key);
int hashtable_int_equal(const void* a, const void* b);

#endif
