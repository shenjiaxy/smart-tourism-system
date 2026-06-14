/**
 * @file test_algorithms.cpp
 * @brief 核心算法模块单元测试 —— 验证所有9个算法的正确性
 *
 * 编译：cmake --build build
 * 运行：./build/bin/SmartTourismSystem --test
 *
 * 测试覆盖：
 *   1. Graph（邻接表图）
 *   2. Heap（最大堆/最小堆/Top-K）
 *   3. Dijkstra（最短路径）
 *   4. TSP（多目标路径规划）
 *   5. HashMap（哈希表）
 *   6. Trie（前缀树）
 *   7. InvertedIndex（倒排索引）
 *   8. Huffman（编码压缩）
 *   9. EditDistance（编辑距离）
 */

#include <iostream>
#include <cstring>
#include <cmath>
#include <cassert>

#include "algorithm/graph.h"
#include "algorithm/heap.h"
#include "algorithm/dijkstra.h"
#include "algorithm/indoor_navigation.h"
#include "algorithm/tsp.h"
#include "algorithm/hash_table.h"
#include "algorithm/trie.h"
#include "algorithm/inverted_index.h"
#include "algorithm/huffman.h"
#include "algorithm/edit_distance.h"
#include "service/route_service.h"
#include "service/auth_service.h"
#include "service/admin_service.h"
#include "service/diary_service.h"

using namespace algorithm;
using namespace std;

// ============================================================
// 测试框架
// ============================================================
static int total_tests = 0;
static int passed_tests = 0;

#define TEST(name) \
    do { \
        total_tests++; \
        cout << "  [TEST] " << name << " ... "; \
    } while(0)

#define PASS() \
    do { \
        passed_tests++; \
        cout << "PASSED" << endl; \
    } while(0)

#define FAIL(msg) \
    do { \
        cout << "FAILED: " << msg << endl; \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { if ((a) != (b)) { FAIL(#a " != " #b); return; } } while(0)

#define ASSERT_NEAR(a, b, eps) \
    do { if (fabs((a) - (b)) > (eps)) { FAIL(#a " != " #b); return; } } while(0)

#define ASSERT(expr) \
    do { if (!(expr)) { FAIL(#expr " is false"); return; } } while(0)

// ============================================================
// 1. Graph 测试
// ============================================================
void test_graph() {
    cout << "\n=== Graph (邻接表图) ===" << endl;

    TEST("基本构建");
    {
        Graph g(Graph::Type::UNDIRECTED);
        g.add_node(0, 0.0, 0.0, -1, "A");
        g.add_node(1, 1.0, 0.0, -1, "B");
        g.add_node(2, 0.5, 1.0, -1, "C");
        g.add_edge(0, 1, 100.0);
        g.add_edge(1, 2, 150.0);
        g.add_edge(0, 2, 200.0);

        ASSERT_EQ(g.node_count(), 3);
        ASSERT_EQ(g.find_node_by_name("B"), 1);
        ASSERT_EQ(g.find_node_by_name("X"), -1);
        PASS();
    }

    TEST("边的数量（无向图）");
    {
        Graph g;
        g.add_node(0, 0, 0);
        g.add_node(1, 1, 0);
        g.add_node(2, 2, 0);
        g.add_edge(0, 1, 10);
        g.add_edge(1, 2, 20);
        // 无向图：每条边存两次
        ASSERT_EQ(g.edge_count(), 4);
        PASS();
    }

    TEST("边的数量（有向图）");
    {
        Graph g(Graph::Type::DIRECTED);
        g.add_node(0, 0, 0);
        g.add_node(1, 1, 0);
        g.add_node(2, 2, 0);
        g.add_edge(0, 1, 10);
        g.add_edge(1, 2, 20);
        ASSERT_EQ(g.edge_count(), 2);
        PASS();
    }
}

// ============================================================
// 2. Heap 测试
// ============================================================
void test_heap() {
    cout << "\n=== Heap (堆) ===" << endl;

    TEST("最大堆基本操作");
    {
        Heap<int, std::less<int>> max_heap;
        max_heap.push(3);
        max_heap.push(1);
        max_heap.push(4);
        max_heap.push(1);
        max_heap.push(5);
        ASSERT_EQ(max_heap.top(), 5);
        ASSERT_EQ(max_heap.pop(), 5);
        ASSERT_EQ(max_heap.pop(), 4);
        ASSERT_EQ(max_heap.pop(), 3);
        ASSERT_EQ(max_heap.size(), 2);
        PASS();
    }

    TEST("最小堆基本操作");
    {
        Heap<int, std::greater<int>> min_heap;
        min_heap.push(3);
        min_heap.push(1);
        min_heap.push(4);
        min_heap.push(1);
        min_heap.push(5);
        ASSERT_EQ(min_heap.top(), 1);
        ASSERT_EQ(min_heap.pop(), 1);
        ASSERT_EQ(min_heap.pop(), 1);
        ASSERT_EQ(min_heap.pop(), 3);
        PASS();
    }

    TEST("Top-K 最大");
    {
        int data[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
        int result[3];
        int k = top_k_largest(data, 10, 3, result);
        ASSERT_EQ(k, 3);
        ASSERT_EQ(result[0], 9);   // 从大到小
        ASSERT_EQ(result[1], 6);
        ASSERT_EQ(result[2], 5);
        PASS();
    }

    TEST("Top-K 最小");
    {
        int data[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
        int result[3];
        int k = top_k_smallest(data, 10, 3, result);
        ASSERT_EQ(k, 3);
        ASSERT_EQ(result[0], 1);   // 从小到大
        ASSERT_EQ(result[1], 1);
        ASSERT_EQ(result[2], 2);
        PASS();
    }

    TEST("建堆");
    {
        int data[] = {5, 3, 8, 1, 9, 2};
        Heap<int, std::less<int>> max_heap;
        max_heap.build_heap(data, 6);
        ASSERT_EQ(max_heap.top(), 9);
        ASSERT_EQ(max_heap.pop(), 9);
        ASSERT_EQ(max_heap.pop(), 8);
        ASSERT_EQ(max_heap.pop(), 5);
        PASS();
    }

    TEST("扩容后保持最小堆顺序");
    {
        Heap<int, std::greater<int>> min_heap(2);
        for (int i = 63; i >= 0; i--) {
            min_heap.push(i);
        }
        for (int i = 0; i < 64; i++) {
            ASSERT_EQ(min_heap.pop(), i);
        }
        ASSERT(min_heap.empty());
        PASS();
    }
}

// ============================================================
// 3. Dijkstra 测试
// ============================================================
void test_dijkstra() {
    cout << "\n=== Dijkstra (最短路径) ===" << endl;

    TEST("简单图最短路径");
    {
        // A --10-- B --5-- C
        // |        |       |
        // 3        2       7
        // |        |       |
        // D --8--- E --1-- F
        Graph g;
        g.add_node(0, 0, 0, -1, "A");  // 0: A
        g.add_node(1, 1, 0, -1, "B");  // 1: B
        g.add_node(2, 2, 0, -1, "C");  // 2: C
        g.add_node(3, 0, 1, -1, "D");  // 3: D
        g.add_node(4, 1, 1, -1, "E");  // 4: E
        g.add_node(5, 2, 1, -1, "F");  // 5: F

        g.add_edge(0, 1, 10);  // A-B
        g.add_edge(0, 3, 3);   // A-D
        g.add_edge(1, 2, 5);   // B-C
        g.add_edge(1, 4, 2);   // B-E
        g.add_edge(2, 5, 7);   // C-F
        g.add_edge(3, 4, 8);   // D-E
        g.add_edge(4, 5, 1);   // E-F

        // A -> C: A-D(3) + D-E(8) + E-F(1) + F-C(7) = 19? NO
        // A-B-C: 10+5=15
        // A-D-E-B-C: 3+8+2+5=18
        // A-D-E-F-C: 3+8+1+7=19
        // 最短: A-B-C = 15
        double dist = Dijkstra::shortest_distance(g, 0, 2);
        ASSERT_NEAR(dist, 15.0, 0.01);
        PASS();
    }

    TEST("路径回溯");
    {
        Graph g;
        g.add_node(0, 0, 0, -1, "A");
        g.add_node(1, 1, 0, -1, "B");
        g.add_node(2, 2, 0, -1, "C");
        g.add_edge(0, 1, 10);
        g.add_edge(1, 2, 5);
        g.add_edge(0, 2, 100);

        int path[10];
        int len = Dijkstra::shortest_path(g, 0, 2, path);
        ASSERT_EQ(len, 3);       // A -> B -> C
        ASSERT_EQ(path[0], 0);   // A
        ASSERT_EQ(path[1], 1);   // B
        ASSERT_EQ(path[2], 2);   // C
        PASS();
    }

    TEST("拥挤度策略");
    {
        Graph g;
        g.add_node(0, 0, 0);  // A
        g.add_node(1, 1, 0);  // B
        g.add_node(2, 2, 0);  // C

        // 两条路 A-B-C: 短但拥挤 vs A-C: 长但畅通
        g.add_edge(0, 1, 100, 0.9, 0);  // A-B: 100m, 非常拥挤
        g.add_edge(1, 2, 100, 0.9, 0);  // B-C: 100m, 非常拥挤
        g.add_edge(0, 2, 300, 0.1, 0);  // A-C: 300m, 畅通

        double dist = Dijkstra::shortest_distance(g, 0, 2, DijkstraStrategy::SHORTEST_DISTANCE);
        ASSERT_NEAR(dist, 200.0, 0.01);  // 距离策略选短路

        double time = Dijkstra::shortest_distance(g, 0, 2, DijkstraStrategy::SHORTEST_TIME);
        // 拥挤路时间: 100/(1.2*0.55) + 100/(1.2*0.55) ≈ 151.5 + 151.5 = 303
        // 畅通路时间: 300/(1.2*0.95) ≈ 263.2
        // 时间策略应选畅通路
        ASSERT(time < 280.0);  // 应该选A-C
        PASS();
    }
}

void test_route_service() {
    cout << "\n=== RouteService (数据库路网) ===" << endl;

    TEST("示范校园单目标最短路径");
    {
        json body = {
            {"area_id", 900001},
            {"from_node", 900005},
            {"to_node", 900014},
            {"strategy", "distance"}
        };
        json result = service::RouteService::plan_single(body);
        ASSERT(result.value("found", false));
        ASSERT(result.value("path_length", 0) >= 2);
        ASSERT(result.value("distance", 0.0) > 0.0);
        PASS();
    }
}

// ============================================================
// 4. TSP 测试
// ============================================================
void test_route_strategy_requirements() {
    cout << "\n=== Route Strategy Requirements ===" << endl;

    TEST("congestion uses actual speed formula");
    {
        Graph g(Graph::Type::DIRECTED);
        g.add_node(0, 0, 0, -1, "A");
        g.add_node(1, 1, 0, -1, "B");
        g.add_edge(0, 1, 100.0, 0.5, 0, 2.0);

        const auto& edge = g.get_neighbors(0)[0];
        ASSERT_NEAR(edge.time, 100.0, 0.01);
        PASS();
    }

    TEST("mixed transport changes mode when faster");
    {
        Graph g(Graph::Type::DIRECTED);
        g.add_node(0, 0, 0, -1, "Gate");
        g.add_node(1, 1, 0, -1, "Transfer");
        g.add_node(2, 2, 0, -1, "Library");

        g.add_edge(0, 1, 60.0, 1.0, 0, 1.0);
        g.add_edge(1, 2, 400.0, 1.0, 1, 10.0);
        g.add_edge(0, 2, 500.0, 1.0, 0, 1.0);

        MixedPathResult mixed = Dijkstra::shortest_mixed_path(g, 0, 2, 0x3);
        ASSERT(mixed.found);
        ASSERT_NEAR(mixed.total_time, 130.0, 0.01);
        ASSERT_EQ(mixed.transfer_count, 1);
        ASSERT_EQ(mixed.path_length, 4);
        ASSERT_EQ(mixed.path[0], 0);
        ASSERT_EQ(mixed.path[1], 1);
        ASSERT_EQ(mixed.path[2], 1);
        ASSERT_EQ(mixed.path[3], 2);
        PASS();
    }
}

void test_indoor_navigation() {
    cout << "\n=== Indoor Navigation ===" << endl;

    TEST("cross-floor route uses elevator connection");
    {
        Graph g(Graph::Type::UNDIRECTED);
        g.add_node(10, 0, 0, 1, "Gate", 1, 1);
        g.add_node(11, 1, 0, 1, "Elevator 1F", 1, 1);
        g.add_node(12, 1, 1, 1, "Elevator 3F", 1, 3);
        g.add_node(13, 2, 1, 1, "Room 302", 1, 3);
        g.add_edge(0, 1, 8.0, 1.0, 0, 1.2);
        g.add_edge(1, 2, 15.0, 1.0, 0, 3.0);
        g.add_edge(2, 3, 10.0, 1.0, 0, 1.2);

        IndoorPathResult route = IndoorNavigation::plan(g, 0, 3);
        ASSERT(route.found);
        ASSERT_EQ(route.path_length, 4);
        ASSERT_EQ(route.floor_changes, 1);
        ASSERT_EQ(route.path[0], 0);
        ASSERT_EQ(route.path[3], 3);
        PASS();
    }
}

void test_tsp() {
    cout << "\n=== TSP (多目标路径) ===" << endl;

    TEST("单途经点");
    {
        // A --5-- B --3-- C --4-- D
        Graph g;
        g.add_node(0, 0, 0, -1, "A");
        g.add_node(1, 1, 0, -1, "B");
        g.add_node(2, 2, 0, -1, "C");
        g.add_node(3, 3, 0, -1, "D");
        g.add_edge(0, 1, 5);
        g.add_edge(1, 2, 3);
        g.add_edge(2, 3, 4);

        int wp[] = {2};  // 途经C
        TSPResult result = TSP::solve(g, 0, 3, wp, 1);
        // A -> C -> D: 路径 A-B-C-D, 距离 12
        ASSERT_NEAR(result.total_dist, 12.0, 0.01);
        PASS();
    }

    TEST("多途经点");
    {
        // 简单方形图
        // 0 --1-- 1
        // |       |
        // 2       3
        // |       |
        // 4 --3-- 5
        Graph g;
        for (int i = 0; i < 6; i++) {
            g.add_node(i, i % 2 * 1.0, (i / 2) * 1.0);
        }
        g.add_edge(0, 1, 1); g.add_edge(0, 2, 2);
        g.add_edge(1, 3, 2); g.add_edge(2, 4, 1);
        g.add_edge(3, 5, 1); g.add_edge(4, 5, 3);

        int wp[] = {3, 4};  // 途经 3,4
        TSPResult result = TSP::solve(g, 0, 5, wp, 2);
        // 路径存在即可（具体顺序取决于启发式）
        ASSERT(result.path_length > 0);
        PASS();
    }
}

// ============================================================
// 5. HashMap 测试
// ============================================================
void test_hashmap() {
    cout << "\n=== HashMap (哈希表) ===" << endl;

    TEST("基本插入和查找");
    {
        HashMap<std::string, int> map;
        map.insert("hello", 1);
        map.insert("world", 2);
        map.insert("test", 3);

        const int* v = map.find("hello");
        ASSERT(v != nullptr);
        ASSERT_EQ(*v, 1);

        ASSERT(map.contains("world"));
        ASSERT(!map.contains("missing"));
        ASSERT_EQ(map.size(), 3);
        PASS();
    }

    TEST("更新已有键");
    {
        HashMap<std::string, int> map;
        map.insert("key", 10);
        map.insert("key", 20);
        const int* v = map.find("key");
        ASSERT_EQ(*v, 20);
        PASS();
    }

    TEST("删除");
    {
        HashMap<std::string, int> map;
        map.insert("a", 1);
        map.insert("b", 2);
        ASSERT(map.erase("a"));
        ASSERT(!map.contains("a"));
        ASSERT_EQ(map.size(), 1);
        PASS();
    }

    TEST("IntHashMap");
    {
        IntHashMap map;
        map.put(1, 100);
        map.put(2, 200);
        map.put(3, 300);
        int* v = map.get(2);
        ASSERT(v != nullptr);
        ASSERT_EQ(*v, 200);
        ASSERT(map.contains(1));
        ASSERT(!map.contains(99));
        PASS();
    }

    TEST("动态扩容");
    {
        HashMap<int, int> map(4);  // 初始4个桶
        for (int i = 0; i < 100; i++) {
            map.insert(i, i * 10);
        }
        ASSERT_EQ(map.size(), 100);
        const int* v = map.find(50);
        ASSERT(v != nullptr);
        ASSERT_EQ(*v, 500);
        PASS();
    }
}

// ============================================================
// 6. Trie 测试
// ============================================================
void test_trie() {
    cout << "\n=== Trie (前缀树) ===" << endl;

    TEST("插入和精确查找");
    {
        Trie t;
        t.insert("hello");
        t.insert("world");
        t.insert("hey");

        ASSERT(t.search("hello"));
        ASSERT(t.search("world"));
        ASSERT(!t.search("hell"));
        ASSERT(!t.search("helloo"));
        PASS();
    }

    TEST("大小写不敏感");
    {
        Trie t;
        t.insert("Hello");
        ASSERT(t.search("hello"));
        ASSERT(t.search("HELLO"));
        PASS();
    }

    TEST("前缀搜索");
    {
        Trie t;
        t.insert("apple");
        t.insert("application");
        t.insert("apply");
        t.insert("banana");

        string results[10];
        int count = t.prefix_search("app", results, 10);
        ASSERT_EQ(count, 3);  // apple, application, apply
        PASS();
    }

    TEST("词频统计");
    {
        Trie t;
        t.insert("hello");
        t.insert("hello");
        t.insert("hello");
        t.insert("world");

        ASSERT_EQ(t.get_count("hello"), 3);
        ASSERT_EQ(t.get_count("world"), 1);
        PASS();
    }

    TEST("starts_with");
    {
        Trie t;
        t.insert("tourism");
        t.insert("tourist");
        ASSERT(t.starts_with("tour"));
        ASSERT(!t.starts_with("travel"));
        PASS();
    }
}

// ============================================================
// 7. InvertedIndex 测试
// ============================================================
void test_inverted_index() {
    cout << "\n=== InvertedIndex (倒排索引) ===" << endl;

    TEST("基本索引和搜索");
    {
        InvertedIndex idx;
        idx.add_document(1, "Great Wall of China", "The Great Wall is one of the greatest wonders of the world");
        idx.add_document(2, "Summer Palace", "Beautiful summer palace with amazing gardens and lakes");
        idx.add_document(3, "Forbidden City", "The Forbidden City is the imperial palace in Beijing");

        int doc_ids[10];
        double scores[10];
        int count = idx.search("great wall", doc_ids, scores, 10);
        ASSERT(count >= 1);
        // 文档1应该排在最前面
        ASSERT_EQ(doc_ids[0], 1);
        PASS();
    }

    TEST("多词AND查询");
    {
        InvertedIndex idx;
        idx.add_document(1, "Beijing Park", "A nice park in Beijing");
        idx.add_document(2, "Shanghai Park", "A large park in Shanghai");
        idx.add_document(3, "Beijing Food", "Great food in Beijing");

        int doc_ids[10];
        double scores[10];
        int count = idx.search("Beijing park", doc_ids, scores, 10);
        ASSERT_EQ(count, 1);
        ASSERT_EQ(doc_ids[0], 1);
        PASS();
    }

    TEST("无结果");
    {
        InvertedIndex idx;
        idx.add_document(1, "Test", "Hello world");

        int doc_ids[10];
        double scores[10];
        int count = idx.search("nonexistent", doc_ids, scores, 10);
        ASSERT_EQ(count, 0);
        PASS();
    }
}

// ============================================================
// 8. Huffman 测试
// ============================================================
void test_huffman() {
    cout << "\n=== Huffman (编码压缩) ===" << endl;

    TEST("基本编码解码");
    {
        const char* text = "hello world";
        int len = static_cast<int>(strlen(text));

        int freq[256];
        Huffman::count_frequency(reinterpret_cast<const unsigned char*>(text), len, freq);

        HuffmanNode* root = Huffman::build_tree(freq);
        ASSERT(root != nullptr);

        HuffmanCode codes[256];
        int code_count = Huffman::build_code_table(root, codes);
        ASSERT(code_count > 0);

        string encoded = Huffman::encode(reinterpret_cast<const unsigned char*>(text), len, codes, code_count);
        ASSERT(!encoded.empty());

        string decoded = Huffman::decode(encoded, root, len);
        ASSERT_EQ(decoded, text);

        Huffman::free_tree(root);
        PASS();
    }

    TEST("单字符文本");
    {
        const char* text = "aaaaa";
        int len = static_cast<int>(strlen(text));

        int freq[256];
        Huffman::count_frequency(reinterpret_cast<const unsigned char*>(text), len, freq);

        HuffmanNode* root = Huffman::build_tree(freq);
        HuffmanCode codes[256];
        Huffman::build_code_table(root, codes);

        string encoded = Huffman::encode(reinterpret_cast<const unsigned char*>(text), len, codes, Huffman::build_code_table(root, codes));
        string decoded = Huffman::decode(encoded, root, len);
        ASSERT_EQ(decoded, text);

        Huffman::free_tree(root);
        PASS();
    }

    TEST("压缩和解压（完整流程）");
    {
        string text = "The quick brown fox jumps over the lazy dog. "
                      "This is a test of Huffman compression algorithm. "
                      "Data structures are fundamental to computer science.";

        const unsigned char* data = reinterpret_cast<const unsigned char*>(text.c_str());
        int len = static_cast<int>(text.size());

        unsigned char* compressed = nullptr;
        int comp_size = Huffman::compress(data, len, &compressed);
        ASSERT(comp_size > 0);

        unsigned char* decompressed = nullptr;
        int decomp_size = Huffman::decompress(compressed, comp_size, &decompressed);
        ASSERT_EQ(decomp_size, len);

        string result(reinterpret_cast<char*>(decompressed), decomp_size);
        ASSERT_EQ(result, text);

        double ratio = Huffman::compression_ratio(len, comp_size);
        cout << "(ratio=" << ratio << ") ";

        delete[] compressed;
        delete[] decompressed;
        PASS();
    }
}

// ============================================================
// 9. EditDistance 测试
// ============================================================
void test_edit_distance() {
    cout << "\n=== EditDistance (编辑距离) ===" << endl;

    TEST("基本编辑距离");
    {
        ASSERT_EQ(EditDistance::compute("kitten", "sitting"), 3);
        ASSERT_EQ(EditDistance::compute("", "abc"), 3);
        ASSERT_EQ(EditDistance::compute("abc", "abc"), 0);
        ASSERT_EQ(EditDistance::compute("abc", ""), 3);
        PASS();
    }

    TEST("相似度计算");
    {
        double sim = EditDistance::similarity("kitten", "sitting");
        ASSERT(sim > 0.5 && sim < 1.0);  // 7字符中差3个，约0.57
        ASSERT_NEAR(EditDistance::similarity("hello", "hello"), 1.0, 0.001);
        ASSERT_NEAR(EditDistance::similarity("", "abc"), 0.0, 0.001);
        PASS();
    }

    TEST("模糊匹配");
    {
        string candidates[] = {"故宫博物院", "国家博物馆", "首都博物馆", "自然博物馆"};
        int idx = EditDistance::fuzzy_match("故宫", candidates, 4, nullptr, 0.3);
        ASSERT_EQ(idx, 0);  // 故宫博物院 最匹配
        PASS();
    }

    TEST("模糊匹配 Top-K");
    {
        string candidates[] = {"清华大学", "北京大学", "复旦 大学", "浙江大学", "南京大学"};
        int indices[3];
        double sims[3];
        int k = EditDistance::fuzzy_match_top_k("北大", candidates, 5, indices, sims, 3, 0.2);
        ASSERT(k >= 1);
        ASSERT_EQ(indices[0], 1);  // 北京大学 最匹配
        PASS();
    }

    TEST("不区分大小写");
    {
        ASSERT_EQ(EditDistance::compute_icase("Hello", "HELLO"), 0);
        ASSERT_EQ(EditDistance::compute_icase("Hello", "hello"), 0);
        PASS();
    }
}

// ============================================================
// 主函数
// ============================================================
// ============================================================
// 主函数：运行所有测试
// ============================================================
void test_auth_service() {
    cout << "\n=== AuthService (login and session) ===" << endl;

    TEST("user login restores session");
    {
        auto result = service::AuthService::login("zhangsan", "pass123");
        ASSERT(result.success);
        ASSERT(!result.token.empty());
        ASSERT_EQ(result.user.username, "zhangsan");
        ASSERT_EQ(result.user.role, "user");

        auto current = service::AuthService::find_session(result.token);
        ASSERT(current.has_value());
        ASSERT_EQ(current->id, result.user.id);
        service::AuthService::logout(result.token);
        PASS();
    }

    TEST("admin role is recognized");
    {
        auto result = service::AuthService::login("admin", "pass123");
        ASSERT(result.success);
        ASSERT(service::AuthService::is_admin(result.user));
        service::AuthService::logout(result.token);
        PASS();
    }

    TEST("wrong password is rejected");
    {
        auto result = service::AuthService::login("zhangsan", "wrong-password");
        ASSERT(!result.success);
        ASSERT(result.token.empty());
        PASS();
    }

    TEST("logout invalidates token");
    {
        auto result = service::AuthService::login("lisi", "pass123");
        ASSERT(result.success);
        service::AuthService::logout(result.token);
        ASSERT(!service::AuthService::find_session(result.token).has_value());
        PASS();
    }

    TEST("different users receive independent session tokens");
    {
        auto user = service::AuthService::login("zhangsan", "pass123");
        auto admin = service::AuthService::login("admin", "pass123");
        ASSERT(user.success);
        ASSERT(admin.success);
        ASSERT(user.token != admin.token);
        auto current_user = service::AuthService::find_session(user.token);
        auto current_admin = service::AuthService::find_session(admin.token);
        ASSERT(current_user.has_value());
        ASSERT(current_admin.has_value());
        ASSERT_EQ(current_user->role, "user");
        ASSERT_EQ(current_admin->role, "admin");
        service::AuthService::logout(user.token);
        service::AuthService::logout(admin.token);
        PASS();
    }

    TEST("session reflects current database role");
    {
        auto result = service::AuthService::login("admin", "pass123");
        ASSERT(result.success);
        ASSERT(service::AdminService::update_user_role(result.user.id, "user"));
        auto current = service::AuthService::find_session(result.token);
        const bool restored = service::AdminService::update_user_role(result.user.id, "admin");
        service::AuthService::logout(result.token);
        ASSERT(restored);
        ASSERT(current.has_value());
        ASSERT_EQ(current->role, "user");
        PASS();
    }
}

void test_admin_service() {
    cout << "\n=== AdminService (admin data) ===" << endl;

    TEST("admin overview and user list");
    {
        auto overview = service::AdminService::get_overview();
        ASSERT(overview.value("users", 0) >= 10);
        ASSERT(overview.value("admins", 0) >= 1);

        auto users = service::AdminService::get_users();
        ASSERT(users.is_array());
        ASSERT(users.size() >= 10);
        PASS();
    }

    TEST("admin updates and restores user role");
    {
        ASSERT(service::AdminService::update_user_role(25, "admin"));
        auto users = service::AdminService::get_users();
        bool promoted = false;
        for (const auto& user : users) {
            if (user.value("id", 0) == 25) {
                promoted = user.value("role", "") == "admin";
                break;
            }
        }
        ASSERT(promoted);
        ASSERT(service::AdminService::update_user_role(25, "user"));
        PASS();
    }
}

void test_diary_service() {
    cout << "\n=== DiaryService (optional fields) ===" << endl;

    TEST("create diary without optional media fields");
    {
        json body = {
            {"user_id", 1},
            {"title", "authorization regression test"},
            {"content", "temporary test diary"},
            {"destination", "test"},
            {"tags", "[]"}
        };
        auto result = service::DiaryService::create_diary(body);
        int diary_id = result.value("id", 0);
        ASSERT(diary_id > 0);
        ASSERT(service::DiaryService::delete_diary(diary_id).value("success", false));
        PASS();
    }
}

void run_all_tests() {
    cout << "========================================" << endl;
    cout << "  Smart Tourism System - Algorithm Tests" << endl;
    cout << "========================================" << endl;

    test_graph();
    test_heap();
    test_dijkstra();
    test_route_strategy_requirements();
    test_indoor_navigation();
    test_tsp();
    test_hashmap();
    test_trie();
    test_inverted_index();
    test_huffman();
    test_edit_distance();
    test_auth_service();
    test_admin_service();
    test_diary_service();

    cout << "\n========================================" << endl;
    cout << "  Results: " << passed_tests << "/" << total_tests << " passed";
    if (passed_tests == total_tests) {
        cout << "  ALL PASSED!" << endl;
    } else {
        cout << "  SOME FAILED!" << endl;
    }
    cout << "========================================" << endl;
}
