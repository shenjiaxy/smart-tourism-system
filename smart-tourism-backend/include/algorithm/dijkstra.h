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

struct MixedPathResult {
    bool found;
    double total_time;
    double total_distance;
    int transfer_count;
    int path_length;
    int* path;
    int* modes;

    MixedPathResult()
        : found(false), total_time(0.0), total_distance(0.0), transfer_count(0),
          path_length(0), path(nullptr), modes(nullptr) {}

    ~MixedPathResult() {
        delete[] path;
        delete[] modes;
    }

    MixedPathResult(const MixedPathResult&) = delete;
    MixedPathResult& operator=(const MixedPathResult&) = delete;

    MixedPathResult(MixedPathResult&& other) noexcept
        : found(other.found), total_time(other.total_time), total_distance(other.total_distance),
          transfer_count(other.transfer_count), path_length(other.path_length),
          path(other.path), modes(other.modes) {
        other.path = nullptr;
        other.modes = nullptr;
        other.path_length = 0;
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

        Heap<DistNode, std::greater<DistNode>> min_heap(n + 1);
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

    /**
     * Run Dijkstra on a three-layer state graph: walking, bicycle and shuttle.
     * A state transition at the same node represents changing transport.
     */
    static MixedPathResult shortest_mixed_path(const Graph& graph, int from_idx, int to_idx,
                                                int allowed_transport = 0x7) {
        MixedPathResult result;
        const int n = graph.node_count();
        if (n == 0 || from_idx < 0 || from_idx >= n || to_idx < 0 || to_idx >= n) {
            return result;
        }

        const int mode_count = 3;
        const int state_count = n * mode_count;
        const double INF = 1e18;
        auto state_of = [mode_count](int node, int mode) { return node * mode_count + mode; };

        auto dist = new double[state_count];
        auto prev = new int[state_count];
        auto visited = new bool[state_count];
        auto available = new bool[state_count];
        for (int i = 0; i < state_count; i++) {
            dist[i] = INF;
            prev[i] = -1;
            visited[i] = false;
            available[i] = false;
        }

        for (int node = 0; node < n; node++) {
            available[state_of(node, 0)] = true;
            available[state_of(node, 1)] = true; // bicycles are available throughout campuses
            const auto& neighbors = graph.get_neighbors(node);
            for (int i = 0; i < neighbors.size(); i++) {
                int mode = neighbors[i].transport;
                if (mode >= 0 && mode < mode_count) {
                    available[state_of(node, mode)] = true;
                    available[state_of(neighbors[i].to, mode)] = true;
                }
            }
        }

        struct StateNode {
            double dist;
            int state;
            bool operator>(const StateNode& other) const { return dist > other.dist; }
            bool operator<(const StateNode& other) const { return dist < other.dist; }
        };
        Heap<StateNode, std::greater<StateNode>> queue(state_count + 1);

        if (allowed_transport & 0x1) {
            int start = state_of(from_idx, 0);
            dist[start] = 0.0;
            queue.push(StateNode{0.0, start});
        } else {
            for (int mode = 1; mode < mode_count; mode++) {
                int start = state_of(from_idx, mode);
                if ((allowed_transport & (1 << mode)) && available[start]) {
                    dist[start] = 0.0;
                    queue.push(StateNode{0.0, start});
                }
            }
        }

        while (!queue.empty()) {
            StateNode current = queue.pop();
            int state = current.state;
            if (visited[state]) continue;
            visited[state] = true;

            int node = state / mode_count;
            int mode = state % mode_count;
            const auto& neighbors = graph.get_neighbors(node);
            for (int i = 0; i < neighbors.size(); i++) {
                const GraphEdge& edge = neighbors[i];
                if (edge.transport != mode) continue;
                int next = state_of(edge.to, mode);
                double candidate = dist[state] + edge.time;
                if (candidate < dist[next]) {
                    dist[next] = candidate;
                    prev[next] = state;
                    queue.push(StateNode{candidate, next});
                }
            }

            for (int next_mode = 0; next_mode < mode_count; next_mode++) {
                if (next_mode == mode || !(allowed_transport & (1 << next_mode))) continue;
                int next = state_of(node, next_mode);
                if (!available[next]) continue;
                double cost = transfer_cost(mode, next_mode);
                if (cost < 0.0) continue;
                double candidate = dist[state] + cost;
                if (candidate < dist[next]) {
                    dist[next] = candidate;
                    prev[next] = state;
                    queue.push(StateNode{candidate, next});
                }
            }
        }

        int best_state = -1;
        double best_time = INF;
        for (int mode = 0; mode < mode_count; mode++) {
            int state = state_of(to_idx, mode);
            if ((allowed_transport & (1 << mode)) && dist[state] < best_time) {
                best_time = dist[state];
                best_state = state;
            }
        }

        if (best_state >= 0 && best_time < INF) {
            int length = 0;
            for (int state = best_state; state >= 0; state = prev[state]) length++;
            result.path = new int[length];
            result.modes = new int[length];
            result.path_length = length;
            result.total_time = best_time;
            result.found = true;

            int state = best_state;
            for (int i = length - 1; i >= 0; i--) {
                result.path[i] = state / mode_count;
                result.modes[i] = state % mode_count;
                state = prev[state];
            }

            for (int i = 0; i + 1 < length; i++) {
                if (result.path[i] == result.path[i + 1]) {
                    if (result.modes[i] != result.modes[i + 1]) result.transfer_count++;
                    continue;
                }
                const auto& neighbors = graph.get_neighbors(result.path[i]);
                for (int j = 0; j < neighbors.size(); j++) {
                    if (neighbors[j].to == result.path[i + 1] &&
                        neighbors[j].transport == result.modes[i]) {
                        result.total_distance += neighbors[j].distance;
                        break;
                    }
                }
            }
        }

        delete[] dist;
        delete[] prev;
        delete[] visited;
        delete[] available;
        return result;
    }

private:
    static double transfer_cost(int from_mode, int to_mode) {
        if (from_mode == to_mode) return 0.0;
        if (from_mode == 0 && to_mode == 1) return 30.0;
        if (from_mode == 1 && to_mode == 0) return 15.0;
        if (from_mode == 0 && to_mode == 2) return 60.0;
        if (from_mode == 2 && to_mode == 0) return 20.0;
        return -1.0;
    }

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
