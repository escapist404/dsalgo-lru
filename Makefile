# 编译器与 C11 编译选项配置
CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
INCLUDES = -Ids -Ilru -Ipr

# 泛型数据结构模块源文件与头文件
DS_SOURCES = ds/seqlist.c ds/linkedlist.c ds/bilinkedlist.c ds/hashtable.c
DS_HEADERS = ds/seqlist.h ds/linkedlist.h ds/bilinkedlist.h ds/hashtable.h

# LRU 缓存实现与交互基准测试模块
LRU_SOURCES = lru/lru.c lru/lru_seqlist.c lru/lru_linkedlist.c \
	lru/lru_hashlist.c
LRU_HEADERS = lru/pair.h lru/lru_seqlist.h lru/lru_linkedlist.h \
	lru/lru_hashlist.h

# 多策略页面置换模拟与对比模块
PAGE_SOURCES = pr/page_replacement.c pr/page_replacement_demo.c
PAGE_HEADERS = pr/page_replacement.h

.PHONY: all clean test

all: lru_demo page_demo

lru_demo: $(LRU_SOURCES) $(DS_SOURCES) $(LRU_HEADERS) $(DS_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LRU_SOURCES) $(DS_SOURCES) -o $@

page_demo: $(PAGE_SOURCES) $(PAGE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) $(PAGE_SOURCES) -o $@

test: lru_demo
	python3 tests/test_file_sequence.py

clean:
	rm -f lru_demo page_demo
