/* 页面置换策略对比实验驱动程序：构造四类典型访存序列并对比五种策略的命中率 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "page_replacement.h"

/* 固定实验参数：缓存容量 16，单组请求数 100，对比策略数 5 */
enum { CAPACITY = 16, REQUESTS = 100, POLICIES = 5 };

static uint32_t next_random(uint32_t *state) {
    if (*state == 0) {
        *state = 1;
    }
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

/* 负载 1：小工作集循环（循环访问页面 1..16，工作集大小等于缓存容量） */
static void make_small_workload(int pages[REQUESTS]) {
    for (size_t i = 0; i < REQUESTS; ++i) {
        pages[i] = (int)(i % CAPACITY) + 1;
    }
}

/* 负载 2：大工作集循环（循环访问页面 1..64，工作集大小为缓存容量的 4 倍） */
static void make_large_workload(int pages[REQUESTS]) {
    for (size_t i = 0; i < REQUESTS; ++i) {
        pages[i] = (int)(i % 64) + 1;
    }
}

/* 负载 3：热点/冷点访问（80% 概率访问热点页 1..16，20% 概率访问冷点页 17..64） */
static size_t make_hot_workload(int pages[REQUESTS]) {
    uint32_t state = 7;
    size_t hot_count = 0;
    for (size_t i = 0; i < REQUESTS; ++i) {
        uint32_t group = next_random(&state);
        uint32_t choice = next_random(&state);
        if (group % 100 < 80) {
            pages[i] = (int)(choice % CAPACITY) + 1;
            ++hot_count;
        } else {
            pages[i] = (int)(choice % 48) + CAPACITY + 1;
        }
    }
    return hot_count;
}

/* 负载 4：均匀随机访问（在页面 1..64 中等概率均匀抽样，无时间局部性） */
static void make_uniform_workload(int pages[REQUESTS]) {
    uint32_t state = 7;
    for (size_t i = 0; i < REQUESTS; ++i) {
        pages[i] = (int)(next_random(&state) % 64) + 1;
    }
}

/* 在给定请求序列上运行指定页面置换策略并返回命中次数 */
static size_t run_policy(const int pages[REQUESTS], PagePolicy policy) {
    PageCache *cache = NULL;
    assert(page_cache_init(&cache, CAPACITY, policy, 7));
    for (size_t i = 0; i < REQUESTS; ++i) {
        page_cache_access(cache, pages[i]);
    }
    size_t hits = page_cache_hits(cache);
    assert(page_cache_accesses(cache) == REQUESTS);
    page_cache_destroy(cache);
    return hits;
}

/* 打印单个工作负载下五种策略的命中统计，并通过断言校验结果可复现性 */
static void print_workload(const char *name, const int pages[REQUESTS],
                           const size_t expected[POLICIES]) {
    printf("%-16s", name);
    for (size_t i = 0; i < POLICIES; ++i) {
        size_t hits = run_policy(pages, (PagePolicy)i);
        assert(hits == expected[i]);
        printf(" %zu/%d (%.4f)", hits, REQUESTS,
               (double)hits / REQUESTS);
    }
    putchar('\n');
}

int main(void) {
    int small[REQUESTS];
    int large[REQUESTS];
    int hot[REQUESTS];
    int uniform[REQUESTS];
    make_small_workload(small);
    make_large_workload(large);
    size_t hot_count = make_hot_workload(hot);
    make_uniform_workload(uniform);

    static const size_t small_expected[POLICIES] = {84, 84, 84, 84, 84};
    static const size_t large_expected[POLICIES] = {0, 0, 0, 2, 15};
    static const size_t hot_expected[POLICIES] = {51, 57, 57, 57, 52};
    static const size_t uniform_expected[POLICIES] = {19, 20, 16, 19, 25};

    printf("capacity=%d requests=%d hot_requests=%zu seed=7\n", CAPACITY,
           REQUESTS, hot_count);
    printf("%-16s", "访问模式");
    for (size_t i = 0; i < POLICIES; ++i) {
        printf(" %-18s", page_policy_name((PagePolicy)i));
    }
    putchar('\n');
    print_workload("小工作集循环", small, small_expected);
    print_workload("大工作集循环", large, large_expected);
    print_workload("热点/冷点访问", hot, hot_expected);
    print_workload("均匀随机访问", uniform, uniform_expected);
    return 0;
}
