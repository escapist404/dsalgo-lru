/* 基于链地址法的泛型哈希表实现：组合 SeqList 桶数组与 LinkedList 冲突链 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

/* 用于向单链表查找回调传递哈希表上下文与目标键 */
typedef struct {
    const HashTable* table;
    const void* key;
} HashQuery;

static int hash_entry_equal(const void* element, const void* target) {
    const HashTableEntry* entry = (const HashTableEntry*)element;
    const HashQuery* query = (const HashQuery*)target;
    return query->table->equal(entry->key, query->key);
}

static size_t bucket_index(const HashTable* table, const void* key) {
    return table->hash(key) % table->bucket_count;
}

/* 获取指定下标的冲突链；若桶为空且 create 非 0 则按需惰性初始化单链表 */
static LinkedList* bucket_at(HashTable* table, size_t index, int create) {
    LinkedList** slot = (LinkedList**)seqlist_at(table->buckets, index);
    if (slot == NULL) {
        return NULL;
    }
    if (*slot == NULL && create &&
        !linkedlist_init(slot, sizeof(HashTableEntry))) {
        return NULL;
    }
    return *slot;
}

static const LinkedList* bucket_at_const(const HashTable* table,
                                         size_t index) {
    const LinkedList* const* slot =
        (const LinkedList* const*)seqlist_at_const(table->buckets, index);
    return slot == NULL ? NULL : *slot;
}

static void free_bucket_entries(LinkedList* bucket) {
    for (LinkedListNode* node = linkedlist_next(linkedlist_head(bucket));
         node != NULL; node = linkedlist_next(node)) {
        HashTableEntry* entry =
            (HashTableEntry*)linkedlist_value(node);
        free(entry->key);
        free(entry->value);
        entry->key = NULL;
        entry->value = NULL;
    }
}

/* 销毁桶数组：rehash 时只销毁链表外骨骼（free_entries = 0），彻底销毁时同时释放键值内存 */
static void destroy_buckets(SeqList* buckets, int free_entries) {
    if (buckets == NULL) {
        return;
    }
    for (size_t i = 0; i < seqlist_length(buckets); ++i) {
        LinkedList** slot = (LinkedList**)seqlist_at(buckets, i);
        if (slot != NULL && *slot != NULL) {
            if (free_entries) {
                free_bucket_entries(*slot);
            }
            linkedlist_destroy(*slot);
        }
    }
    seqlist_destroy(buckets);
}

static SeqList* create_buckets(size_t bucket_count) {
    SeqList* buckets = NULL;
    if (!seqlist_init(&buckets, sizeof(LinkedList*), bucket_count)) {
        return NULL;
    }
    LinkedList* empty = NULL;
    for (size_t i = 0; i < bucket_count; ++i) {
        if (!seqlist_push_back(buckets, &empty)) {
            seqlist_destroy(buckets);
            return NULL;
        }
    }
    return buckets;
}

static LinkedListNode* find_entry_node(HashTable* table, const void* key) {
    LinkedList* bucket = bucket_at(table, bucket_index(table, key), 0);
    if (bucket == NULL) {
        return NULL;
    }
    HashQuery query = {table, key};
    LinkedListNode* previous =
        linkedlist_find_previous(bucket, &query, hash_entry_equal);
    return previous == NULL ? NULL : linkedlist_next(previous);
}

static const LinkedListNode* find_entry_node_const(const HashTable* table,
                                                   const void* key) {
    const LinkedList* bucket =
        bucket_at_const(table, bucket_index(table, key));
    if (bucket == NULL) {
        return NULL;
    }
    HashQuery query = {table, key};
    LinkedListNode* previous =
        linkedlist_find_previous(bucket, &query, hash_entry_equal);
    return previous == NULL ? NULL : linkedlist_next(previous);
}

/* 扩容并重新分桶：复用原有 HashTableEntry 的 key/value 指针，避免重复拷贝键值数据 */
static int rehash(HashTable* table, size_t new_bucket_count) {
    SeqList* new_buckets = create_buckets(new_bucket_count);
    if (new_buckets == NULL) {
        return 0;
    }

    for (size_t i = 0; i < seqlist_length(table->buckets); ++i) {
        LinkedList** old_slot = (LinkedList**)seqlist_at(table->buckets, i);
        if (old_slot == NULL || *old_slot == NULL) {
            continue;
        }
        for (LinkedListNode* node =
                 linkedlist_next(linkedlist_head(*old_slot));
             node != NULL; node = linkedlist_next(node)) {
            HashTableEntry* entry =
                (HashTableEntry*)linkedlist_value(node);
            size_t index = table->hash(entry->key) % new_bucket_count;
            LinkedList** new_slot =
                (LinkedList**)seqlist_at(new_buckets, index);
            if (*new_slot == NULL &&
                !linkedlist_init(new_slot, sizeof(HashTableEntry))) {
                destroy_buckets(new_buckets, 0);
                return 0;
            }
            if (!linkedlist_push_front(*new_slot, entry)) {
                destroy_buckets(new_buckets, 0);
                return 0;
            }
        }
    }

    destroy_buckets(table->buckets, 0);
    table->buckets = new_buckets;
    table->bucket_count = new_bucket_count;
    return 1;
}

int hashtable_init(HashTable** out, size_t key_size, size_t value_size,
                   HashTableHashFunc hash, HashTableEqualFunc equal) {
    if (out == NULL || key_size == 0 || value_size == 0 || hash == NULL ||
        equal == NULL) {
        return 0;
    }
    *out = NULL;

    HashTable* table = (HashTable*)calloc(1, sizeof(*table));
    if (table == NULL) {
        return 0;
    }
    /* 初始分配 16 个哈希桶 */
    table->buckets = create_buckets(16);
    if (table->buckets == NULL) {
        free(table);
        return 0;
    }
    table->bucket_count = 16;
    table->key_size = key_size;
    table->value_size = value_size;
    table->hash = hash;
    table->equal = equal;
    *out = table;
    return 1;
}

void hashtable_destroy(HashTable* table) {
    if (table == NULL) {
        return;
    }
    destroy_buckets(table->buckets, 1);
    free(table);
}

void* hashtable_get(HashTable* table, const void* key) {
    if (table == NULL || key == NULL) {
        return NULL;
    }
    LinkedListNode* node = find_entry_node(table, key);
    if (node == NULL) {
        return NULL;
    }
    return ((HashTableEntry*)linkedlist_value(node))->value;
}

const void* hashtable_get_const(const HashTable* table, const void* key) {
    if (table == NULL || key == NULL) {
        return NULL;
    }
    const LinkedListNode* node = find_entry_node_const(table, key);
    if (node == NULL) {
        return NULL;
    }
    return ((const HashTableEntry*)linkedlist_value_const(node))->value;
}

int hashtable_put(HashTable* table, const void* key, const void* value) {
    if (table == NULL || key == NULL || value == NULL) {
        return 0;
    }

    /* 若键已存在则原地覆盖更新 value */
    LinkedListNode* node = find_entry_node(table, key);
    if (node != NULL) {
        HashTableEntry* entry =
            (HashTableEntry*)linkedlist_value(node);
        memcpy(entry->value, value, table->value_size);
        return 1;
    }

    /* 装载因子超过 0.75 时将桶数量翻倍并重新哈希 */
    if ((table->size + 1) * 4 > table->bucket_count * 3 &&
        !rehash(table, table->bucket_count * 2)) {
        return 0;
    }

    HashTableEntry entry = {0};
    entry.key = malloc(table->key_size);
    entry.value = malloc(table->value_size);
    if (entry.key == NULL || entry.value == NULL) {
        free(entry.key);
        free(entry.value);
        return 0;
    }
    memcpy(entry.key, key, table->key_size);
    memcpy(entry.value, value, table->value_size);

    LinkedList* bucket = bucket_at(table, bucket_index(table, key), 1);
    if (bucket == NULL || !linkedlist_push_front(bucket, &entry)) {
        free(entry.key);
        free(entry.value);
        return 0;
    }
    ++table->size;
    return 1;
}

int hashtable_remove(HashTable* table, const void* key) {
    if (table == NULL || key == NULL) {
        return 0;
    }
    LinkedList* bucket = bucket_at(table, bucket_index(table, key), 0);
    if (bucket == NULL) {
        return 0;
    }
    HashQuery query = {table, key};
    LinkedListNode* previous =
        linkedlist_find_previous(bucket, &query, hash_entry_equal);
    if (previous == NULL) {
        return 0;
    }
    LinkedListNode* node = linkedlist_next(previous);
    HashTableEntry* entry = (HashTableEntry*)linkedlist_value(node);
    free(entry->key);
    free(entry->value);
    entry->key = NULL;
    entry->value = NULL;
    linkedlist_erase_after(bucket, previous, NULL);
    --table->size;
    return 1;
}

size_t hashtable_size(const HashTable* table) {
    return table == NULL ? 0 : table->size;
}

int hashtable_empty(const HashTable* table) {
    return table == NULL || table->size == 0;
}

/* 32 位整数雪崩混淆哈希函数，降低规律性整型键的桶冲突率 */
size_t hashtable_int_hash(const void* key) {
    uint32_t value = (uint32_t)*(const int*)key;
    value ^= value >> 16;
    value *= UINT32_C(0x7feb352d);
    value ^= value >> 15;
    value *= UINT32_C(0x846ca68b);
    value ^= value >> 16;
    return (size_t)value;
}

int hashtable_int_equal(const void* a, const void* b) {
    return *(const int*)a == *(const int*)b;
}
