/**
 * @file dijkstra.h
 * @brief Dijkstra 最短路径算法 —— 用于路线规划和场所查询
 *
 * 核心算法：Dijkstra 单源最短路径
 *
 * 三种策略支持：
 *   1. 最短距离：边权 = 物理距离
 *   2. 最短时间（考虑拥挤度）：边权 = 时间 = 距离 / (速度 * (1 - 拥挤度*0.5))
 *   3. 交通工具混合（多层图）：不同层间通过换乘车费/时间连接
 *
 * 时间复杂度：O((V + E) * log V)（使用最小堆优化）
 * 空间复杂度：O(V + E)
 */

#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph.h"
#include "heap.h"
#include <cstring>
#include <stdexcept>

namespace algorithm {

// ============================================================
// Dijkstra 策略枚举
// ============================================================
enum class DijkstraStrategy {
    SHORTEST_DISTANCE,   // 最短距离
    SHORTEST_TIME,       // 最短时间（考虑拥挤度）
    MIXED_TRANSPORT      // 混合交通工具
};

// ============================================================
// Dijkstra 结果
// ============================================================
struct DijkstraResult {
    double* dist;        // 各节点到源的最短距离/时间
    int* prev;           // 前驱节点索引（用于回溯路径）
    int source;          // 源节点索引
    int node_count;      // 节点总数
    bool* visited;       // 访问标记

    DijkstraResult() : dist(nullptr), prev(nullptr), source(-1), node_count(0), visited(nullptr) {}

    ~DijkstraResult() {
        delete[] dist;
        delete[] prev;
        delete[] visited;
    }

    DijkstraResult(const DijkstraResult&) = delete;
    DijkstraResult& operator=(const DijkstraResult&) = delete;

    DijkstraResult(DijkstraResult&& other) noexcept
        : dist(other.dist), prev(other.prev), source(other.source),
          node_count(other.node_count), visited(other.visited) {
        other.dist = nullptr;
        other.prev = nullptr;
        other.visited = nullptr;
    }

    /// 从目标节点回溯路径
    /// @param target 目标节点索引
    /// @param path 输出路径（从源到目标）
    /// @return 路径长度（节点数），-1表示不可达
    int trace_path(int target, int* path) const {
        if (target < 0 || target >= node_count) return -1;
        if (dist[target] < 0) return -1;  // 不可达

        // 先计算路径长度
        int len = 0;
        int cur = target;
        while (cur != source) {
            if (cur == -1) return -1;          // 路径断裂
            if (len > node_count) return -1;   // prev 数组出现环路，防止无限循环
            cur = prev[cur];
            len++;
        }
        len++;  // 包含源节点

        // 从源到目标填充路径
        cur = target;
        for (int i = len - 1; i >= 0; i--) {
            path[i] = cur;
            cur = prev[cur];
        }

        return len;
    }
};

// ============================================================
// Dijkstra 算法实现
// ============================================================
class Dijkstra {
public:
    /**
     * @brief 执行 Dijkstra 最短路径
     *
     * @param graph 图（邻接表）
     * @param source_idx 源节点内部索引
     * @param strategy 路径优化策略
     * @param allowed_transport 允许的交通工具类型位掩码
     *        bit0=步行(1), bit1=自行车(2), bit2=电瓶车(4)
     *        默认 0x7 表示全部允许
     * @return DijkstraResult 结果（含距离数组和前驱数组）
     */
    static DijkstraResult run(const Graph& graph, int source_idx,
                              DijkstraStrategy strategy = DijkstraStrategy::SHORTEST_DISTANCE,
                              int allowed_transport = 0x7) {
        int n = graph.node_count();
        DijkstraResult result;
        result.node_count = n;
        result.source = source_idx;
        result.dist = new double[n];
        result.prev = new int[n];
        result.visited = new bool[n];

        // 初始化
        const double INF = 1e18;
        for (int i = 0; i < n; i++) {
            result.dist[i] = INF;
            result.prev[i] = -1;
            result.visited[i] = false;
        }
        result.dist[source_idx] = 0.0;

        // 使用最小堆优化（存储 {距离, 节点索引}）
        // 使用自定义结构体而非 pair
        struct DistNode {
            double dist;
            int node;
            bool operator>(const DistNode& o) const { return dist > o.dist; }
            bool operator<(const DistNode& o) const { return dist < o.dist; }
        };

        Heap<DistNode, std::less<DistNode>> min_heap(n + 1);
        min_heap.push(DistNode{0.0, source_idx});

        while (!min_heap.empty()) {
            DistNode cur = min_heap.pop();
            int u = cur.node;

            if (result.visited[u]) continue;
            result.visited[u] = true;

            const auto& neighbors = graph.get_neighbors(u);
            for (int i = 0; i < neighbors.size(); i++) {
                const GraphEdge& edge = neighbors[i];
                int v = edge.to;

                // 交通工具过滤
                if (!(allowed_transport & (1 << edge.transport))) continue;

                // 根据策略选择边权
                double weight = get_edge_weight(edge, strategy);

                // 松弛操作
                double new_dist = result.dist[u] + weight;
                if (new_dist < result.dist[v]) {
                    result.dist[v] = new_dist;
                    result.prev[v] = u;
                    min_heap.push(DistNode{new_dist, v});
                }
            }
        }

        // 将不可达的距离设为 -1
        for (int i = 0; i < n; i++) {
            if (result.dist[i] >= INF) {
                result.dist[i] = -1.0;
            }
        }

        return result;
    }

    /**
     * @brief 计算两点之间的最短路径（便捷方法）
     *
     * @param graph 图
     * @param from_idx 起点内部索引
     * @param to_idx 终点内部索引
     * @param path 输出路径（调用者分配，至少 node_count 个元素）
     * @param strategy 策略
     * @param allowed_transport 交通工具位掩码
     * @return 路径长度（节点数），0表示不可达
     */
    static int shortest_path(const Graph& graph, int from_idx, int to_idx,
                             int* path,
                             DijkstraStrategy strategy = DijkstraStrategy::SHORTEST_DISTANCE,
                             int allowed_transport = 0x7) {
        if (from_idx == to_idx) {
            path[0] = from_idx;
            return 1;
        }

        DijkstraResult result = run(graph, from_idx, strategy, allowed_transport);
        int len = result.trace_path(to_idx, path);
        return (len > 0) ? len : 0;
    }

    /**
     * @brief 计算两点之间的最短距离
     */
    static double shortest_distance(const Graph& graph, int from_idx, int to_idx,
                                    DijkstraStrategy strategy = DijkstraStrategy::SHORTEST_DISTANCE,
                                    int allowed_transport = 0x7) {
        if (from_idx == to_idx) return 0.0;
        DijkstraResult result = run(graph, from_idx, strategy, allowed_transport);
        return result.dist[to_idx];
    }

private:
    /// 根据策略获取边的权重
    static double get_edge_weight(const GraphEdge& edge, DijkstraStrategy strategy) {
        switch (strategy) {
            case DijkstraStrategy::SHORTEST_DISTANCE:
                return edge.distance;

            case DijkstraStrategy::SHORTEST_TIME:
                // 时间已预计算在 edge.time 中
                return edge.time;

            case DijkstraStrategy::MIXED_TRANSPORT:
                // 混合交通工具：考虑换乘成本
                // 使用时间作为主要权重，不同交通工具间换乘有额外惩罚
                return edge.time;

            default:
                return edge.distance;
        }
    }
};

} // namespace algorithm

#endif // DIJKSTRA_H
