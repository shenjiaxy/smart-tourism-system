/**
 * @file tsp.h
 * @brief TSP（旅行商问题）近似算法 —— 用于多目标途经最短路径
 *
 * 算法方案：最近邻启发式 + 2-opt 局部优化
 *
 * 问题定义：
 *   给定起点、终点和若干途经点，找出经过所有途经点的最短路径。
 *   这是开放路径TSP（起点和终点可不同）。
 *
 * 算法步骤：
 *   1. 最近邻启发式：从起点开始，每次选择最近的未访问途经点
 *   2. 2-opt 优化：反复交换路径中的两条边，若能缩短路径则保留
 *
 * 时间复杂度：
 *   最近邻: O(n^2)（n为途经点数）
 *   2-opt:  O(n^2 * iterations)
 *   总体近似比: 2-opt 保证在最优解的 1.5 倍以内
 */

#ifndef TSP_H
#define TSP_H

#include "graph.h"
#include "dijkstra.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace algorithm {

// ============================================================
// TSP 路径结果
// ============================================================
struct TSPResult {
    int* path;          // 完整路径（节点索引序列）
    int path_length;    // 路径经过的节点数
    double total_dist;  // 总距离/时间
    double total_time;  // 总时间

    TSPResult() : path(nullptr), path_length(0), total_dist(0), total_time(0) {}

    ~TSPResult() { delete[] path; }

    TSPResult(const TSPResult&) = delete;
    TSPResult& operator=(const TSPResult&) = delete;

    TSPResult(TSPResult&& other) noexcept
        : path(other.path), path_length(other.path_length),
          total_dist(other.total_dist), total_time(other.total_time) {
        other.path = nullptr;
        other.path_length = 0;
    }
};

// ============================================================
// TSP 近似算法
// ============================================================
class TSP {
public:
    /**
     * @brief 最近邻启发式 + 2-opt 优化
     *
     * @param graph 图（邻接表）
     * @param start 起点节点索引
     * @param end 终点节点索引（-1表示回到起点）
     * @param waypoints 途经点数组（节点索引）
     * @param waypoint_count 途经点数量
     * @param strategy 路径策略（最短距离/最短时间）
     * @param max_2opt_iter 2-opt最大迭代次数
     * @return TSPResult 路径结果
     */
    static TSPResult solve(const Graph& graph,
                           int start, int end,
                           const int* waypoints, int waypoint_count,
                           DijkstraStrategy strategy = DijkstraStrategy::SHORTEST_DISTANCE,
                           int max_2opt_iter = 100) {
        TSPResult result;

        if (waypoint_count == 0) {
            // 无途经点，直接计算起点到终点的最短路径
            result.path = new int[graph.node_count()];
            result.path_length = Dijkstra::shortest_path(graph, start, end, result.path, strategy);
            if (result.path_length > 0) {
                result.total_dist = calc_path_distance(graph, result.path, result.path_length);
                result.total_time = calc_path_time(graph, result.path, result.path_length);
            }
            return result;
        }

        if (waypoint_count == 1) {
            // 单个途经点：start -> waypoint -> end
            return solve_two_stage(graph, start, waypoints[0], end, strategy);
        }

        // ---- 预计算：所有关键点之间的最短距离 ----
        // 关键点包括：起点 + 所有途经点 + 终点
        int key_count = waypoint_count + 2;
        int* key_nodes = new int[key_count];
        key_nodes[0] = start;
        for (int i = 0; i < waypoint_count; i++) {
            key_nodes[i + 1] = waypoints[i];
        }
        key_nodes[key_count - 1] = (end >= 0) ? end : start;

        // 预计算距离矩阵 dist_matrix[i][j] = key_nodes[i] 到 key_nodes[j] 的最短距离
        double* dist_matrix = new double[key_count * key_count];
        for (int i = 0; i < key_count; i++) {
            DijkstraResult dijk = Dijkstra::run(graph, key_nodes[i], strategy);
            for (int j = 0; j < key_count; j++) {
                // dijk.dist 以图节点索引寻址，必须用 key_nodes[j] 取对应节点的距离
                dist_matrix[i * key_count + j] = dijk.dist[key_nodes[j]];
            }
        }

        // ---- 最近邻启发式 ----
        int* order = new int[waypoint_count];  // 途经点的访问顺序
        bool* used = new bool[waypoint_count];
        memset(used, false, sizeof(bool) * waypoint_count);

        int current_key = 0;  // 从起点开始（key索引0）
        for (int step = 0; step < waypoint_count; step++) {
            double best_dist = 1e18;
            int best_wp = -1;
            for (int w = 0; w < waypoint_count; w++) {
                if (used[w]) continue;
                double d = dist_matrix[current_key * key_count + (w + 1)];  // key索引 = w+1
                if (d >= 0 && d < best_dist) {
                    best_dist = d;
                    best_wp = w;
                }
            }
            if (best_wp >= 0) {
                order[step] = best_wp;
                used[best_wp] = true;
                current_key = best_wp + 1;  // 转换回key索引
            }
        }

        // ---- 2-opt 局部优化 ----
        // 传终点在 key 数组中的索引（key_count-1），而非图节点编号
        two_opt_optimize(order, waypoint_count, dist_matrix, key_count,
                         key_count - 1, max_2opt_iter);

        // ---- 拼接完整路径 ----
        // 路径：start -> order[0] -> order[1] -> ... -> order[n-1] -> end
        // 每段用 Dijkstra 获取实际节点路径
        // 共 waypoint_count+1 段，每段最多 node_count 个节点
        result.path = new int[graph.node_count() * (waypoint_count + 1) + 10];
        result.path_length = 0;
        result.total_dist = 0;

        // 临时路径缓冲区
        int* seg_path = new int[graph.node_count()];

        // start -> 第一个途经点
        int from = start;
        for (int step = 0; step < waypoint_count; step++) {
            int to = waypoints[order[step]];
            int seg_len = Dijkstra::shortest_path(graph, from, to, seg_path, strategy);

            if (seg_len > 0) {
                // 拼接（避免重复节点）
                int start_idx = 0;
                if (result.path_length > 0 && seg_path[0] == result.path[result.path_length - 1]) {
                    start_idx = 1;  // 跳过重复的连接点
                }
                for (int i = start_idx; i < seg_len; i++) {
                    result.path[result.path_length++] = seg_path[i];
                }
                result.total_dist += calc_path_distance(graph, seg_path, seg_len);
            }
            from = to;
        }

        // 最后一个途经点 -> 终点
        int final_dest = (end >= 0) ? end : start;
        int seg_len = Dijkstra::shortest_path(graph, from, final_dest, seg_path, strategy);
        if (seg_len > 0) {
            int start_idx = 0;
            if (result.path_length > 0 && seg_path[0] == result.path[result.path_length - 1]) {
                start_idx = 1;
            }
            for (int i = start_idx; i < seg_len; i++) {
                result.path[result.path_length++] = seg_path[i];
            }
            result.total_dist += calc_path_distance(graph, seg_path, seg_len);
        }

        result.total_time = calc_path_time(graph, result.path, result.path_length);

        // 清理
        delete[] key_nodes;
        delete[] dist_matrix;
        delete[] order;
        delete[] used;
        delete[] seg_path;

        return result;
    }

private:
    /// 两段式路径（起点 -> 途经点 -> 终点）
    static TSPResult solve_two_stage(const Graph& graph, int start, int waypoint, int end,
                                     DijkstraStrategy strategy) {
        TSPResult result;
        result.path = new int[graph.node_count() * 2 + 10];
        result.path_length = 0;
        result.total_dist = 0;

        int* seg = new int[graph.node_count()];

        // start -> waypoint
        int len1 = Dijkstra::shortest_path(graph, start, waypoint, seg, strategy);
        if (len1 > 0) {
            for (int i = 0; i < len1; i++) {
                result.path[result.path_length++] = seg[i];
            }
            result.total_dist += calc_path_distance(graph, seg, len1);
        }

        // waypoint -> end
        int len2 = Dijkstra::shortest_path(graph, waypoint, end, seg, strategy);
        if (len2 > 0) {
            int start_idx = 0;
            if (result.path_length > 0 && seg[0] == result.path[result.path_length - 1]) {
                start_idx = 1;
            }
            for (int i = start_idx; i < len2; i++) {
                result.path[result.path_length++] = seg[i];
            }
            result.total_dist += calc_path_distance(graph, seg, len2);
        }

        result.total_time = calc_path_time(graph, result.path, result.path_length);

        delete[] seg;
        return result;
    }

    /// 2-opt 优化：交换路径中的两段，看是否能缩短总距离
    static void two_opt_optimize(int* order, int n,
                                 const double* dist_matrix, int key_count,
                                 int end_key, int max_iter) {
        bool improved = true;
        int iter = 0;

        while (improved && iter < max_iter) {
            improved = false;
            iter++;

            for (int i = 0; i < n - 1; i++) {
                for (int j = i + 2; j < n; j++) {
                    // 计算交换前后的距离差
                    // 交换前: ... A - order[i] -> order[i+1] - ... - order[j] -> B ...
                    // 交换后: ... A - order[j] -> order[j-1] - ... - order[i] -> B ...
                    int a = (i == 0) ? 0 : order[i - 1] + 1;   // key索引
                    int b = (j == n - 1) ? end_key : order[j + 1] + 1;  // key索引
                    int ci = order[i] + 1;   // key索引
                    int cj = order[j] + 1;   // key索引

                    double before = dist_matrix[a * key_count + ci]
                                  + dist_matrix[cj * key_count + b];
                    double after  = dist_matrix[a * key_count + cj]
                                  + dist_matrix[ci * key_count + b];

                    if (after < before - 1e-9) {
                        // 反转 order[i..j]
                        reverse_range(order, i, j);
                        improved = true;
                    }
                }
            }
        }
    }

    /// 反转数组中 [lo, hi] 范围
    static void reverse_range(int* arr, int lo, int hi) {
        while (lo < hi) {
            int tmp = arr[lo];
            arr[lo] = arr[hi];
            arr[hi] = tmp;
            lo++;
            hi--;
        }
    }

    /// 计算路径上的实际距离（沿边累加）
    static double calc_path_distance(const Graph& graph, const int* path, int path_len) {
        if (path_len <= 1) return 0.0;
        double total = 0.0;
        for (int i = 0; i < path_len - 1; i++) {
            const auto& neighbors = graph.get_neighbors(path[i]);
            for (int j = 0; j < neighbors.size(); j++) {
                if (neighbors[j].to == path[i + 1]) {
                    total += neighbors[j].distance;
                    break;
                }
            }
        }
        return total;
    }

    /// 计算路径上的总时间
    static double calc_path_time(const Graph& graph, const int* path, int path_len) {
        if (path_len <= 1) return 0.0;
        double total = 0.0;
        for (int i = 0; i < path_len - 1; i++) {
            const auto& neighbors = graph.get_neighbors(path[i]);
            for (int j = 0; j < neighbors.size(); j++) {
                if (neighbors[j].to == path[i + 1]) {
                    total += neighbors[j].time;
                    break;
                }
            }
        }
        return total;
    }
};

} // namespace algorithm

#endif // TSP_H
