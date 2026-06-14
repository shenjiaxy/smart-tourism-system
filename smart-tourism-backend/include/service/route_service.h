/**
 * @file route_service.h
 * @brief 路线规划服务 —— Dijkstra最短路径 + TSP多目标路径规划
 *
 * 核心算法：
 *   - Dijkstra: 单源最短路径，支持距离/时间/混合交通三种策略
 *   - TSP: 最近邻启发式 + 2-opt局部优化的多目标途经路径
 */

#pragma once

#include "json.hpp"
#include "algorithm/graph.h"
#include "algorithm/dijkstra.h"
#include "algorithm/tsp.h"
#include "algorithm/hash_table.h"
#include "repository/spot_repo.h"
#include <string>
#include <cmath>
#include <memory>

using json = nlohmann::json;

namespace service {

class RouteService {
public:
    /**
     * 单目标最短路径
     * Request body: {
     *   "area_id": int,
     *   "from_node": int,
     *   "to_node": int,
     *   "strategy": "distance" | "time" | "mixed",
     *   "transport": int bitmask (default 7 = all)
     * }
     */
    static json plan_single(const json& body) {
        int area_id = body.value("area_id", 0);
        int from_node = body.value("from_node", 0);
        int to_node = body.value("to_node", 0);
        std::string strategy_str = body.value("strategy", std::string("distance"));
        int transport = body.value("transport", 0);

        if (area_id <= 0 || from_node <= 0 || to_node <= 0) {
            return {{"error", "area_id, from_node, to_node are required"}};
        }

        // 构建图（有向图：单向道路只加正向边，双向道路加两条方向相反的边）
        algorithm::Graph g(algorithm::Graph::Type::DIRECTED);
        auto id_to_idx = build_graph(g, area_id);
        if (id_to_idx.size() == 0) {
            return {{"error", "No graph data found for area_id"}, {"area_id", area_id}};
        }

        // 查找节点索引
        auto from_it = id_to_idx.find(from_node);
        auto to_it = id_to_idx.find(to_node);
        if (!from_it.valid || !to_it.valid) {
            return {{"error", "Node not found in graph"}, {"from_node", from_node}, {"to_node", to_node}};
        }

        // 选择策略
        algorithm::DijkstraStrategy strategy = algorithm::DijkstraStrategy::SHORTEST_DISTANCE;
        if (strategy_str == "time") strategy = algorithm::DijkstraStrategy::SHORTEST_TIME;
        else if (strategy_str == "mixed") strategy = algorithm::DijkstraStrategy::MIXED_TRANSPORT;

        if (transport == 0) {
            json area = repository::SpotRepo::get_by_id(area_id);
            transport = area.value("type", std::string("scenic")) == "campus" ? 0x3 : 0x5;
        }

        if (strategy == algorithm::DijkstraStrategy::MIXED_TRANSPORT) {
            algorithm::MixedPathResult mixed = algorithm::Dijkstra::shortest_mixed_path(
                g, from_it.value, to_it.value, transport);
            if (!mixed.found) {
                return {{"found", false}, {"message", "No mixed-transport path found"}};
            }

            json path_ids = json::array();
            json path_names = json::array();
            json modes = json::array();
            json transfers = json::array();
            for (int i = 0; i < mixed.path_length; i++) {
                const auto& node = g.get_node(mixed.path[i]);
                path_ids.push_back(node.id);
                path_names.push_back(node.name);
                modes.push_back(mixed.modes[i]);
                if (i > 0 && mixed.path[i] == mixed.path[i - 1] &&
                    mixed.modes[i] != mixed.modes[i - 1]) {
                    transfers.push_back({
                        {"node_id", node.id},
                        {"node_name", node.name},
                        {"from_transport", mixed.modes[i - 1]},
                        {"to_transport", mixed.modes[i]}
                    });
                }
            }

            return {
                {"found", true},
                {"path", path_ids},
                {"path_names", path_names},
                {"node_names", path_names},
                {"transport_modes", modes},
                {"transfers", transfers},
                {"transfer_count", mixed.transfer_count},
                {"path_length", mixed.path_length},
                {"distance", mixed.total_distance},
                {"time", mixed.total_time},
                {"strategy", strategy_str}
            };
        }

        // 计算最短路径
        int path[1024];
        int path_len = algorithm::Dijkstra::shortest_path(g, from_it.value, to_it.value,
                                                          path, strategy, transport);

        json result;
        if (path_len == 0) {
            result["found"] = false;
            result["message"] = "No path found between the two nodes";
        } else {
            result["found"] = true;
            // 路径节点ID序列
            json path_ids = json::array();
            json path_names = json::array();
            for (int i = 0; i < path_len; i++) {
                const auto& node = g.get_node(path[i]);
                path_ids.push_back(node.id);
                path_names.push_back(node.name);
            }
            result["path"] = path_ids;
            result["path_names"] = path_names;
            result["node_names"] = path_names;
            result["path_length"] = path_len;

            // 计算距离和时间
            double total_dist = calc_path_distance(g, path, path_len);
            double total_time = calc_path_time(g, path, path_len);
            result["distance"] = total_dist;
            result["time"] = total_time;
            result["strategy"] = strategy_str;
        }

        return result;
    }

    /**
     * 多目标TSP路径
     * Request body: {
     *   "area_id": int,
     *   "from_node": int,
     *   "to_node": int,
     *   "waypoints": [int, ...],
     *   "strategy": "distance" | "time"
     * }
     */
    static json plan_multi(const json& body) {
        int area_id = body.value("area_id", 0);
        int from_node = body.value("from_node", 0);
        int to_node = body.value("to_node", -1);
        std::string strategy_str = body.value("strategy", std::string("distance"));

        if (!body.contains("waypoints") || !body["waypoints"].is_array()) {
            return {{"error", "waypoints array is required"}};
        }

        // 构建图（有向图：单向道路只加正向边，双向道路加两条方向相反的边）
        algorithm::Graph g(algorithm::Graph::Type::DIRECTED);
        auto id_to_idx = build_graph(g, area_id);
        if (id_to_idx.size() == 0) {
            return {{"error", "No graph data found for area_id"}};
        }

        auto from_it = id_to_idx.find(from_node);
        if (!from_it.valid) {
            return {{"error", "Start node not found"}};
        }

        int end_idx = from_it.value;  // 默认回到起点
        if (to_node > 0) {
            auto to_it = id_to_idx.find(to_node);
            if (to_it.valid) end_idx = to_it.value;
        }

        // 提取途经点索引
        int wp_count = static_cast<int>(body["waypoints"].size());
        auto wp_indices = std::make_unique<int[]>(wp_count);
        for (int i = 0; i < wp_count; i++) {
            int wp_id = body["waypoints"][i].get<int>();
            auto it = id_to_idx.find(wp_id);
            if (!it.valid) {
                return {{"error", "Waypoint not found"}, {"waypoint_id", wp_id}};
            }
            wp_indices[i] = it.value;
        }

        algorithm::DijkstraStrategy strategy = algorithm::DijkstraStrategy::SHORTEST_DISTANCE;
        if (strategy_str == "time") strategy = algorithm::DijkstraStrategy::SHORTEST_TIME;

        // 求解TSP
        algorithm::TSPResult tsp_result = algorithm::TSP::solve(
            g, from_it.value, end_idx, wp_indices.get(), wp_count, strategy);

        json result;
        if (tsp_result.path_length == 0) {
            result["found"] = false;
            result["message"] = "No feasible path found";
        } else {
            result["found"] = true;
            json path_ids = json::array();
            json path_names = json::array();
            for (int i = 0; i < tsp_result.path_length; i++) {
                const auto& node = g.get_node(tsp_result.path[i]);
                path_ids.push_back(node.id);
                path_names.push_back(node.name);
            }
            result["path"] = path_ids;
            result["path_names"] = path_names;
            result["node_names"] = path_names;
            result["path_length"] = tsp_result.path_length;
            result["distance"] = tsp_result.total_dist;
            result["time"] = tsp_result.total_time;
        }

        return result;
    }

    /**
     * 获取路网数据（供前端Canvas绘制）
     */
    static json get_graph_data(int area_id) {
        json nodes = repository::SpotRepo::get_nodes(area_id);
        json roads = repository::SpotRepo::get_roads(area_id);

        json result;
        result["area_id"] = area_id;
        result["nodes"] = nodes;
        result["edges"] = roads;
        result["roads"] = roads;
        result["node_count"] = nodes.size();
        result["edge_count"] = roads.size();
        return result;
    }

private:
    /**
     * 从数据库构建 Graph 对象
     * 返回 node_db_id -> graph_index 的映射
     *
     * 注意：图使用有向模式，单向道路（is_bidirectional=0）只加一条方向边，
     * 双向道路显式加两条方向相反的边。这样路线规划结果才能正确反映
     * 现实中的单行道约束。
     */
    static algorithm::IntHashMap build_graph(algorithm::Graph& g, int area_id) {
        algorithm::IntHashMap id_to_idx(4096);

        // 加载节点
        json nodes = repository::SpotRepo::get_nodes(area_id);
        for (const auto& n : nodes) {
            int db_id = n["id"].get<int>();
            std::string name = n.value("name", std::string(""));
            double pos_x = n.value("pos_x", 0.0);
            double pos_y = n.value("pos_y", 0.0);
            std::string type = n.value("type", std::string("junction"));
            std::string sub_type = n.value("sub_type", std::string(""));
            int floor_num = n.value("floor", -1);

            int node_type = 0;  // junction
            if (type == "building") node_type = 1;
            else if (type == "facility") node_type = 2;
            else if (type == "entrance") node_type = 1;

            int idx = g.add_node(db_id, pos_x, pos_y, area_id, name, node_type, floor_num);
            id_to_idx.put(db_id, idx);
        }

        // 加载边
        json roads = repository::SpotRepo::get_roads(area_id);
        for (const auto& r : roads) {
            int from_db = r["from_node"].get<int>();
            int to_db = r["to_node"].get<int>();
            double distance = r.value("distance", 0.0);
            double congestion = r.value("congestion", 0.5);
            double ideal_speed = r.value("ideal_speed", 0.0);
            int transport = r.value("transport", 0);
            int is_bidir = r.value("is_bidirectional", 1);

            int* from_idx = id_to_idx.get(from_db);
            int* to_idx = id_to_idx.get(to_db);
            if (from_idx && to_idx) {
                // 总是添加正向边
                g.add_edge(*from_idx, *to_idx, distance, congestion, transport, ideal_speed);
                // 双向道路：额外添加反向边
                if (is_bidir != 0) {
                    g.add_edge(*to_idx, *from_idx, distance, congestion, transport, ideal_speed);
                }
            }
        }

        return id_to_idx;
    }

    static double calc_path_distance(const algorithm::Graph& g, const int* path, int len) {
        if (len <= 1) return 0.0;
        double total = 0.0;
        for (int i = 0; i < len - 1; i++) {
            const auto& neighbors = g.get_neighbors(path[i]);
            for (int j = 0; j < neighbors.size(); j++) {
                if (neighbors[j].to == path[i + 1]) {
                    total += neighbors[j].distance;
                    break;
                }
            }
        }
        return total;
    }

    static double calc_path_time(const algorithm::Graph& g, const int* path, int len) {
        if (len <= 1) return 0.0;
        double total = 0.0;
        for (int i = 0; i < len - 1; i++) {
            const auto& neighbors = g.get_neighbors(path[i]);
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

} // namespace service
