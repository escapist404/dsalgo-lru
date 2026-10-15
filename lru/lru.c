/* LRU 缓存交互式驱动程序与多方案性能对比基准测试框架 */
#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lru_hashlist.h"
#include "lru_linkedlist.h"
#include "lru_seqlist.h"

/* 缓存访问操作类型：GET 查询、PUT 写入与 STATE 状态输出 */
typedef enum {
    OP_GET,
    OP_PUT,
    OP_STATE
} OperationKind;

/* 单条缓存访问指令（用于批量性能基准测试） */
typedef struct {
    OperationKind kind;
    PairType pair;
} Operation;

/* 单个方案在基准测试序列上的耗时与命中统计结果 */
typedef struct {
    uint64_t elapsed_ns;      /* 总执行耗时（纳秒） */
    size_t hit_count;         /* GET 命中次数 */
    size_t missed_count;      /* GET 未命中次数 */
    size_t access_count;      /* GET 总请求次数 */
    size_t replacement_count; /* 缓存淘汰次数 */
} BenchmarkResult;

/* 向动态操作序列数组追加一条指令（容量不足时按 2 倍扩容） */
static int append_operation(Operation** operations, size_t* length,
                            size_t* capacity, Operation operation) {
    if (*length == *capacity) {
        size_t new_capacity = *capacity == 0 ? 64 : *capacity * 2;
        if (new_capacity < *capacity ||
            new_capacity > SIZE_MAX / sizeof(**operations)) {
            return 0;
        }
        Operation* resized = (Operation*)realloc(
            *operations, new_capacity * sizeof(**operations));
        if (resized == NULL) {
            return 0;
        }
        *operations = resized;
        *capacity = new_capacity;
    }
    (*operations)[(*length)++] = operation;
    return 1;
}

/* 交互式读取正整数（size_t） */
static int prompt_size(const char* prompt, size_t* result) {
    char line[128];
    unsigned long long value;
    char extra;

    fputs(prompt, stdout);
    if (fgets(line, sizeof(line), stdin) == NULL) {
        return -1;
    }
    if (sscanf(line, "%llu %c", &value, &extra) != 1 || value == 0 ||
        value > SIZE_MAX) {
        return 0;
    }
    *result = (size_t)value;
    return 1;
}

/* 交互式读取有符号整数（int） */
static int prompt_int(const char* prompt, int* result) {
    char line[128];
    long long value;
    char extra;

    fputs(prompt, stdout);
    if (fgets(line, sizeof(line), stdin) == NULL) {
        return -1;
    }
    if (sscanf(line, "%lld %c", &value, &extra) != 1 || value < INT_MIN ||
        value > INT_MAX) {
        return 0;
    }
    *result = (int)value;
    return 1;
}

/* 交互式读取文件路径并去除末尾换行符 */
static int prompt_path(const char* prompt, char* path, size_t path_size) {
    fputs(prompt, stdout);
    if (fgets(path, path_size, stdin) == NULL) {
        return -1;
    }
    path[strcspn(path, "\r\n")] = '\0';
    return path[0] != '\0';
}

/* 确定性 xorshift32 伪随机数发生器，保证跨运行实验的可复现性 */
static uint32_t next_random(uint32_t* state) {
    if (*state == 0) {
        *state = 1;
    }
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

/* 生成指定长度的伪随机访问序列（60% 概率为 GET，40% 概率为 PUT） */
static int generate_random_sequence(size_t count, size_t key_range,
                                    uint32_t seed, Operation** out,
                                    size_t* out_length) {
    Operation* operations = NULL;
    size_t length = 0;
    size_t capacity = 0;
    if (key_range == 0 || key_range > INT_MAX) {
        return 0;
    }

    for (size_t i = 0; i < count; ++i) {
        uint32_t random_value = next_random(&seed);
        Operation operation;
        operation.kind = random_value % 100 < 60 ? OP_GET : OP_PUT;
        operation.pair.key = (int)(next_random(&seed) % key_range);
        operation.pair.value = (int)(next_random(&seed) % 1000000u);
        if (!append_operation(&operations, &length, &capacity, operation)) {
            free(operations);
            return 0;
        }
    }
    *out = operations;
    *out_length = length;
    return 1;
}

/* 解析逐行操作；可选首行“容量 操作数”，忽略空行与 # 注释 */
static int load_sequence(const char* path, Operation** out, size_t* out_length,
                         size_t* cache_size) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return 0;
    }

    Operation* operations = NULL;
    size_t length = 0;
    size_t capacity = 0;
    char line[256];
    size_t line_number = 0;
    size_t expected_length = SIZE_MAX;
    int success = 1;

    while (fgets(line, sizeof(line), file) != NULL) {
        ++line_number;
        char command[8] = {0};
        long long key = 0;
        long long value = 0;
        char extra;
        int fields = sscanf(line, "%7s %lld %lld %c", command, &key, &value,
                            &extra);
        if (fields == EOF || command[0] == '#') {
            continue;
        }
        if (length == 0 && expected_length == SIZE_MAX &&
            sscanf(line, "%lld %lld %c", &key, &value, &extra) == 2 &&
            key > 0 && (unsigned long long)key <= SIZE_MAX && value >= 0 &&
            (unsigned long long)value < SIZE_MAX) {
            *cache_size = (size_t)key;
            expected_length = (size_t)value;
            continue;
        }
        Operation operation = {0};
        if (strcmp(command, "GET") == 0 && fields == 2 &&
            sscanf(line, "%7s %lld %c", command, &key, &extra) == 2 &&
            key >= INT_MIN && key <= INT_MAX) {
            operation.kind = OP_GET;
            operation.pair.key = (int)key;
            operation.pair.value = 0;
        } else if (strcmp(command, "PUT") == 0 && fields == 3 &&
                   key >= INT_MIN && key <= INT_MAX &&
                   value >= INT_MIN && value <= INT_MAX) {
            operation.kind = OP_PUT;
            operation.pair.key = (int)key;
            operation.pair.value = (int)value;
        } else if (strcmp(command, "STATE") == 0 && fields == 1 &&
                   sscanf(line, "%7s %c", command, &extra) == 1) {
            operation.kind = OP_STATE;
        } else {
            fprintf(stderr, "invalid operation at %s:%zu\n", path, line_number);
            success = 0;
            break;
        }
        if (!append_operation(&operations, &length, &capacity, operation)) {
            success = 0;
            break;
        }
    }
    if (ferror(file) || *cache_size == 0 ||
        (expected_length != SIZE_MAX && length != expected_length)) {
        success = 0;
    }
    fclose(file);

    if (!success) {
        free(operations);
        return 0;
    }
    *out = operations;
    *out_length = length;
    return 1;
}

/* 获取单调递增时钟的纳秒时间戳 */
static uint64_t clock_ns(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t)now.tv_sec * UINT64_C(1000000000) + (uint64_t)now.tv_nsec;
}

/* 在完全相同的操作序列上依次评测顺序表、单链表和哈希+双链表三种 LRU 实现 */
static int benchmark_sequence(const Operation* operations, size_t length,
                              size_t cache_size, BenchmarkResult* array_result,
                              BenchmarkResult* linked_result,
                              BenchmarkResult* hash_result) {
    LRUSeqList* array = NULL;
    LRULinkedList* linked = NULL;
    LRUHashList* hash = NULL;
    if (!lru_seqlist_init(&array, cache_size) ||
        !lru_linkedlist_init(&linked, cache_size) ||
        !lru_hashlist_init(&hash, cache_size)) {
        lru_seqlist_destroy(array);
        lru_linkedlist_destroy(linked);
        lru_hashlist_destroy(hash);
        return 0;
    }

    uint64_t start = clock_ns();
    for (size_t i = 0; i < length; ++i) {
        if (operations[i].kind == OP_GET) {
            PairElemType key = operations[i].pair.key;
            lru_seqlist_get(array, &key, NULL);
        } else if (operations[i].kind == OP_PUT) {
            lru_seqlist_put(array, (PairType*)&operations[i].pair);
        }
    }
    array_result->elapsed_ns = clock_ns() - start;
    array_result->hit_count = array->hit_count;
    array_result->missed_count = array->missed_count;
    array_result->access_count = array->access_count;
    array_result->replacement_count = array->replacement_count;

    start = clock_ns();
    for (size_t i = 0; i < length; ++i) {
        if (operations[i].kind == OP_GET) {
            PairElemType key = operations[i].pair.key;
            lru_linkedlist_get(linked, &key, NULL);
        } else if (operations[i].kind == OP_PUT) {
            lru_linkedlist_put(linked, (PairType*)&operations[i].pair);
        }
    }
    linked_result->elapsed_ns = clock_ns() - start;
    linked_result->hit_count = linked->hit_count;
    linked_result->missed_count = linked->missed_count;
    linked_result->access_count = linked->access_count;
    linked_result->replacement_count = linked->replacement_count;

    start = clock_ns();
    for (size_t i = 0; i < length; ++i) {
        if (operations[i].kind == OP_GET) {
            PairElemType key = operations[i].pair.key;
            lru_hashlist_get(hash, &key, NULL);
        } else if (operations[i].kind == OP_PUT) {
            lru_hashlist_put(hash, (PairType*)&operations[i].pair);
        }
    }
    hash_result->elapsed_ns = clock_ns() - start;
    hash_result->hit_count = hash->hit_count;
    hash_result->missed_count = hash->missed_count;
    hash_result->access_count = hash->access_count;
    hash_result->replacement_count = hash->replacement_count;

    lru_seqlist_destroy(array);
    lru_linkedlist_destroy(linked);
    lru_hashlist_destroy(hash);
    return 1;
}

static double hit_rate(const BenchmarkResult* result) {
    return result->access_count == 0
               ? 0.0
               : (double)result->hit_count / result->access_count;
}

/* 格式化打印三种方案的性能对比表格 */
static void print_comparison(size_t operation_count,
                             const BenchmarkResult* array_result,
                             const BenchmarkResult* linked_result,
                             const BenchmarkResult* hash_result) {
    printf("\n%-24s %18s %18s %18s\n", "指标", "方案A 数组",
           "方案B 单链表", "方案C 哈希+双链表");
    printf("%-24s %15.3f ms %15.3f ms %15.3f ms\n", "总耗时",
           array_result->elapsed_ns / 1000000.0,
           linked_result->elapsed_ns / 1000000.0,
           hash_result->elapsed_ns / 1000000.0);
    printf("%-24s %18.6f %18.6f %18.6f\n", "命中率", hit_rate(array_result),
           hit_rate(linked_result), hit_rate(hash_result));
    printf("%-24s %15.3f ns %15.3f ns %15.3f ns\n", "平均每次操作耗时",
           operation_count == 0
               ? 0.0
               : (double)array_result->elapsed_ns / operation_count,
           operation_count == 0
               ? 0.0
               : (double)linked_result->elapsed_ns / operation_count,
           operation_count == 0
               ? 0.0
               : (double)hash_result->elapsed_ns / operation_count);
    printf("%-24s %18zu %18zu %18zu\n", "GET 次数", array_result->access_count,
           linked_result->access_count, hash_result->access_count);
    printf("%-24s %18zu %18zu %18zu\n", "淘汰次数",
           array_result->replacement_count, linked_result->replacement_count,
           hash_result->replacement_count);
}

static void run_benchmark(const Operation* operations, size_t length,
                          size_t cache_size) {
    BenchmarkResult array_result;
    BenchmarkResult linked_result;
    BenchmarkResult hash_result;
    if (!benchmark_sequence(operations, length, cache_size, &array_result,
                             &linked_result, &hash_result)) {
        puts("benchmark initialization failed");
        return;
    }
    print_comparison(length, &array_result, &linked_result, &hash_result);
}

static void benchmark_random(size_t cache_size) {
    size_t count;
    size_t key_range;
    size_t seed_value;
    int result = prompt_size("操作数量 (>0): ", &count);
    if (result != 1) {
        puts(result < 0 ? "输入结束" : "数量不合法");
        return;
    }
    result = prompt_size("key 范围 (>0): ", &key_range);
    if (result != 1) {
        puts(result < 0 ? "输入结束" : "key 范围不合法");
        return;
    }
    result = prompt_size("随机种子 (>0): ", &seed_value);
    if (result != 1 || seed_value > UINT32_MAX) {
        puts("随机种子不合法");
        return;
    }

    Operation* operations = NULL;
    size_t length = 0;
    if (!generate_random_sequence(count, key_range, (uint32_t)seed_value,
                                  &operations, &length)) {
        puts("随机序列生成失败");
        return;
    }
    run_benchmark(operations, length, cache_size);
    free(operations);
}

/* 文件测试使用独立空缓存，仅输出 GET、STATE 结果与末尾统计 */
static int run_file(const char* path, size_t cache_size, int active) {
    Operation* operations = NULL;
    size_t length = 0;
    if (!load_sequence(path, &operations, &length, &cache_size)) {
        fputs("读取访问序列失败\n", stderr);
        return 1;
    }

    LRUSeqList* array = NULL;
    LRULinkedList* linked = NULL;
    LRUHashList* hash = NULL;
    int initialized = active == 1 ? lru_seqlist_init(&array, cache_size)
                                 : active == 2
                                       ? lru_linkedlist_init(&linked, cache_size)
                                       : lru_hashlist_init(&hash, cache_size);
    if (!initialized) {
        free(operations);
        fputs("初始化失败\n", stderr);
        return 1;
    }

    int status = 0;
    for (size_t i = 0; i < length; ++i) {
        Operation* operation = &operations[i];
        if (operation->kind == OP_GET) {
            int value = -1;
            if (active == 1) {
                lru_seqlist_get(array, &operation->pair.key, &value);
            } else if (active == 2) {
                lru_linkedlist_get(linked, &operation->pair.key, &value);
            } else {
                lru_hashlist_get(hash, &operation->pair.key, &value);
            }
            printf("%d\n", value);
        } else if (operation->kind == OP_PUT) {
            int inserted = active == 1 ? lru_seqlist_put(array, &operation->pair)
                                      : active == 2
                                            ? lru_linkedlist_put(linked, &operation->pair)
                                            : lru_hashlist_put(hash, &operation->pair);
            if (!inserted) {
                fputs("PUT 失败\n", stderr);
                status = 1;
                break;
            }
        } else if (active == 1) {
            lru_seqlist_state(array);
        } else if (active == 2) {
            lru_linkedlist_state(linked);
        } else {
            lru_hashlist_state(hash);
        }
    }
    if (active == 1) {
        lru_seqlist_stats(array);
    } else if (active == 2) {
        lru_linkedlist_stats(linked);
    } else {
        lru_hashlist_stats(hash);
    }
    lru_seqlist_destroy(array);
    lru_linkedlist_destroy(linked);
    lru_hashlist_destroy(hash);
    free(operations);
    return status;
}

static void test_file(size_t cache_size, int active) {
    char path[256];
    int result = prompt_path("访问序列文件: ", path, sizeof(path));
    if (result != 1) {
        puts(result < 0 ? "输入结束" : "文件名不能为空");
        return;
    }
    putchar('\n');
    run_file(path, cache_size, active);
}

/* 打印当前选中的 LRU 方案缓存状态 */
static void print_active_state(int active, const LRUSeqList* array,
                               const LRULinkedList* linked,
                               const LRUHashList* hash) {
    if (active == 1) {
        lru_seqlist_state(array);
    } else if (active == 2) {
        lru_linkedlist_state(linked);
    } else {
        lru_hashlist_state(hash);
    }
}

int main(int argc, char** argv) {
    if (argc != 1) {
        int active = 1;
        if ((argc != 3 && argc != 4) || strcmp(argv[1], "--file") != 0 ||
            (argc == 4 && strcmp(argv[3], "1") != 0 &&
             strcmp(argv[3], "2") != 0 && strcmp(argv[3], "3") != 0)) {
            fputs("用法: lru_demo --file <文件> [1|2|3]\n", stderr);
            return 1;
        }
        if (argc == 4) {
            active = argv[3][0] - '0';
        }
        return run_file(argv[2], 0, active);
    }
    size_t cache_size;
    int result;
    do {
        result = prompt_size("缓存容量 (>0): ", &cache_size);
        if (result == 0) {
            puts("容量必须是正整数");
        }
    } while (result == 0);
    if (result < 0) {
        return 0;
    }

    LRUSeqList* array = NULL;
    LRULinkedList* linked = NULL;
    LRUHashList* hash = NULL;
    if (!lru_seqlist_init(&array, cache_size) ||
        !lru_linkedlist_init(&linked, cache_size) ||
        !lru_hashlist_init(&hash, cache_size)) {
        lru_seqlist_destroy(array);
        lru_linkedlist_destroy(linked);
        lru_hashlist_destroy(hash);
        fputs("初始化失败\n", stderr);
        return 1;
    }

    int active = 1;
    int print_after = 0;
    char line[128];
    for (;;) {
        printf("\n当前方案: %s | 操作后打印: %s\n",
               active == 1 ? "方案A 数组"
                           : active == 2 ? "方案B 单链表" : "方案C 哈希+双链表",
               print_after ? "开" : "关");
        puts("1. 选择数据结构");
        puts("2. GET");
        puts("3. PUT");
        puts("4. 打印当前缓存");
        puts("5. 随机访问序列性能比较");
        puts("6. 文件访问序列测试");
        puts("7. 切换操作后打印");
        puts("0. 退出");
        fputs("选择: ", stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        int choice;
        if (sscanf(line, "%d", &choice) != 1) {
            puts("请输入菜单编号");
            continue;
        }

        if (choice == 0) {
            break;
        } else if (choice == 1) {
            int selected;
            result = prompt_int("选择 1=数组，2=单链表，3=哈希+双链表: ",
                                &selected);
            if (result == 1 && (selected == 1 || selected == 2 || selected == 3)) {
                active = selected;
            } else if (result >= 0) {
                puts("数据结构选择不合法");
            }
        } else if (choice == 2) {
            int key;
            result = prompt_int("key: ", &key);
            if (result != 1) {
                puts(result < 0 ? "输入结束" : "key 不合法");
                continue;
            }
            int value = -1;
            int found;
            if (active == 1) {
                found = lru_seqlist_get(array, &key, &value);
            } else if (active == 2) {
                found = lru_linkedlist_get(linked, &key, &value);
            } else {
                found = lru_hashlist_get(hash, &key, &value);
            }
            printf("%s: %d\n", found ? "命中" : "未命中", value);
            if (print_after) {
                print_active_state(active, array, linked, hash);
            }
        } else if (choice == 3) {
            PairType pair;
            result = prompt_int("key: ", &pair.key);
            if (result != 1) {
                puts(result < 0 ? "输入结束" : "key 不合法");
                continue;
            }
            result = prompt_int("value: ", &pair.value);
            if (result != 1) {
                puts(result < 0 ? "输入结束" : "value 不合法");
                continue;
            }
            int inserted = active == 1 ? lru_seqlist_put(array, &pair)
                                       : active == 2
                                             ? lru_linkedlist_put(linked, &pair)
                                             : lru_hashlist_put(hash, &pair);
            puts(inserted ? "PUT 完成" : "PUT 失败");
            if (print_after) {
                print_active_state(active, array, linked, hash);
            }
        } else if (choice == 4) {
            print_active_state(active, array, linked, hash);
        } else if (choice == 5) {
            benchmark_random(cache_size);
        } else if (choice == 6) {
            test_file(cache_size, active);
        } else if (choice == 7) {
            print_after = !print_after;
        } else {
            puts("未知菜单选项");
        }
    }

    lru_seqlist_destroy(array);
    lru_linkedlist_destroy(linked);
    lru_hashlist_destroy(hash);
    return 0;
}
