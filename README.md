# LRU 缓存模拟器与页面置换算法实验 (`dsalgo-lru`)

本项目基于 C11 标准自底向上实现了四个泛型基础数据结构容器，并在此基础上构建了三种不同底层结构的 **LRU（Least Recently Used）缓存模拟器**，以及涵盖五种经典策略（**FIFO、LRU、LFU、Random、MRU**）的**页面置换模拟器**。

完整的实验设计、复杂度推导与性能评测分析详见根目录报告：
* [实验 1 报告：基于线性表的 LRU 缓存模拟器设计与实现 (report.md)](./report.md)

## 目录结构

```text
.
├── Makefile                    # 顶层构建脚本（构建 lru_demo 与 page_demo）
├── README.md                   # 项目说明文档
├── report.md                   # 实验报告（设计、复杂度推导与性能分析）
├── ds/                         # 泛型基础数据结构模块
│   ├── seqlist.h / seqlist.c           # 泛型动态顺序表 SeqList
│   ├── linkedlist.h / linkedlist.c     # 带头结点的泛型单链表 LinkedList
│   ├── bilinkedlist.h / bilinkedlist.c # 带头尾哨兵的泛型双向链表 BiLinkedList
│   └── hashtable.h / hashtable.c       # 链地址法泛型哈希表 HashTable
├── lru/                        # LRU 缓存实现与基准测试模块
│   ├── pair.h                          # 缓存键值对定义 PairType (int -> int)
│   ├── lru_seqlist.h / lru_seqlist.c   # 方案 A：基于顺序表的 LRU (O(k) 查找/搬移)
│   ├── lru_linkedlist.h / lru_linkedlist.c # 方案 B：基于单链表的 LRU (O(k) 查找/淘汰, O(1) 前移)
│   ├── lru_hashlist.h / lru_hashlist.c # 方案 C：基于哈希表+双向链表的 LRU (平均 O(1))
│   └── lru.c                           # 交互式命令行程序与多方案性能基准测试
└── pr/                         # 多策略页面置换模拟与对比模块
    ├── page_replacement.h / page_replacement.c # FIFO / LRU / LFU / Random / MRU 统一模拟框架
    └── page_replacement_demo.c                 # 四类典型访存负载下的命中率对比实验
```

## 编译与运行

### 环境要求

* 支持 C11 标准的编译器（`clang` 或 `gcc`）
* `make` 构建工具

### 构建命令

```bash
# 同时编译 lru_demo 与 page_demo
make

# 仅编译 LRU 缓存模拟器
make lru_demo

# 仅编译页面置换策略对比程序
make page_demo

# 清理编译产物
make clean
```

## 功能说明与使用指南

### 1. LRU 缓存交互与基准测试（`./lru_demo`）

运行 `./lru_demo` 后，首先输入正整数缓存容量 $k$，随后可通过交互式菜单操作：

* **`1. 选择数据结构`**：在方案 A（顺序表）、方案 B（单链表）和方案 C（哈希表 + 双向链表）之间切换；
* **`2. GET` / `3. PUT`**：单步执行缓存查询或写入更新；
* **`4. 打印当前缓存`**：按从新到旧顺序输出当前缓存内容（格式：`STATE key:value ...`，空缓存输出 `EMPTY`）；
* **`5. 随机访问序列性能比较`**：输入操作数量、Key 范围和随机种子，使用确定性 `xorshift32` 生成相同操作序列，对比三种方案的总耗时（`ms`）、单次操作耗时（`ns`）、命中率与淘汰次数；
* **`6. 文件访问序列测试`**：使用当前选择的方案，从空缓存执行文件中的操作，输出查询、状态与最终统计；
* **`7. 切换操作后打印`**：开启或关闭每次 `GET` / `PUT` 后自动打印缓存状态；
* **`0. 退出`**：释放所有内存并退出。

#### 访问序列文件格式

文件首个有效行为 `容量 操作数`，之后每行一条指令，支持空行与 `#` 开头的注释行：

```text
# 示例访问序列
2 5
GET 1
PUT 1 10
PUT 2 20
GET 1
STATE
```

`GET` 输出查询到的 value，未命中输出 `-1`；`PUT` 不输出；`STATE` 按从新到旧的顺序输出 `STATE key:value ...`，空缓存只输出 `EMPTY`。文件执行结束后自动输出最后一行 `STATS 命中次数 未命中次数 淘汰次数 命中率`，命中率为命中次数除以 GET 总次数，保留六位小数，无 GET 时为 `0.000000`。无需在输入中写 `STATS`。

可直接运行文件测试，使标准输出只包含上述结果，便于重定向或比对：

```bash
./lru_demo --file sequence.in > sequence.out     # 默认方案 A（方法 1）
./lru_demo --file sequence.in 3 > sequence.out   # 方案 C（1=A、2=B、3=C）
```

以上示例的输出为：

```text
-1
10
STATE 1:10 2:20
STATS 1 1 0 0.500000
```

命令行文件测试从首个有效行读取容量，并校验操作总数（包括 STATE），无需传入容量。空行与注释不计入操作数。通过交互菜单运行时，也兼容不带该首行的文件，使用菜单中设置的容量。

运行文件测试的回归检查：

```bash
make test
```

### 2. 页面置换策略对比实验（`./page_demo`）

直接运行 `./page_demo` 即可在固定容量（$k = 16$）与 $100$ 次请求下，自动评测 **FIFO、LRU、LFU、Random、MRU** 五种页面置换策略在四类工作负载（小工作集循环、大工作集循环、热点/冷点访问、均匀随机访问）下的命中次数与命中率，并执行断言校验：

```bash
./page_demo
```
