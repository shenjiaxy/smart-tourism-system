#pragma once

#include "json.hpp"
#include "algorithm/graph.h"
#include "algorithm/hash_table.h"
#include "algorithm/indoor_navigation.h"
#include "repository/indoor_repo.h"
#include <string>

using json = nlohmann::json;

namespace service {

class IndoorRouteService {
public:
    static json get_buildings(int area_id) {
        if (area_id <= 0) return {{"error", "area_id is required"}};
        return repository::IndoorRepo::get_buildings(area_id);
    }

    static json get_graph(int building_id) {
        json nodes = repository::IndoorRepo::get_nodes(building_id);
        json roads = repository::IndoorRepo::get_roads(building_id);
        return {
            {"building_id", building_id},
            {"nodes", nodes},
            {"roads", roads},
            {"node_count", nodes.size()},
            {"road_count", roads.size()}
        };
    }

    static json plan(const json& body) {
        int building_id = body.value("building_id", 0);
        int from_node = body.value("from_node", 0);
        int to_node = body.value("to_node", 0);
        std::string strategy_text = body.value("strategy", std::string("distance"));
        if (building_id <= 0 || from_node <= 0 || to_node <= 0) {
            return {{"error", "building_id, from_node and to_node are required"}};
        }

        json nodes = repository::IndoorRepo::get_nodes(building_id);
        json roads = repository::IndoorRepo::get_roads(building_id);
        if (nodes.empty()) return {{"error", "No indoor data found for building"}};

        algorithm::Graph graph(algorithm::Graph::Type::UNDIRECTED);
        algorithm::IntHashMap id_to_idx(512);
        for (const auto& node : nodes) {
            int id = node.value("id", 0);
            int idx = graph.add_node(id, node.value("pos_x", 0.0), node.value("pos_y", 0.0),
                                     building_id, node.value("name", std::string("")), 1,
                                     node.value("floor", 1));
            id_to_idx.put(id, idx);
        }

        for (const auto& road : roads) {
            int* from = id_to_idx.get(road.value("from_node", 0));
            int* to = id_to_idx.get(road.value("to_node", 0));
            if (!from || !to) continue;
            double distance = road.value("distance", 0.0);
            int from_floor = graph.get_node(*from).floor_num;
            int to_floor = graph.get_node(*to).floor_num;
            double speed = (from_floor == to_floor) ? 1.2 : 2.5;
            graph.add_edge(*from, *to, distance, 1.0, 0, speed);
        }

        int* from_idx = id_to_idx.get(from_node);
        int* to_idx = id_to_idx.get(to_node);
        if (!from_idx || !to_idx) return {{"error", "Indoor node not found"}};

        algorithm::DijkstraStrategy strategy = strategy_text == "time"
            ? algorithm::DijkstraStrategy::SHORTEST_TIME
            : algorithm::DijkstraStrategy::SHORTEST_DISTANCE;
        algorithm::IndoorPathResult route = algorithm::IndoorNavigation::plan(
            graph, *from_idx, *to_idx, strategy);
        if (!route.found) return {{"found", false}, {"message", "No indoor path found"}};

        json path = json::array();
        json path_names = json::array();
        json floors = json::array();
        json steps = json::array();
        double total_distance = 0.0;
        double total_time = 0.0;

        for (int i = 0; i < route.path_length; i++) {
            const auto& node = graph.get_node(route.path[i]);
            path.push_back(node.id);
            path_names.push_back(node.name);
            floors.push_back(node.floor_num);
            if (i == 0) {
                steps.push_back("从 " + node.name + " 出发");
                continue;
            }

            const auto& previous = graph.get_node(route.path[i - 1]);
            if (previous.floor_num != node.floor_num) {
                std::string connector = previous.name + node.name;
                std::string action = connector.find("楼梯") != std::string::npos ? "走楼梯" : "乘电梯";
                steps.push_back(action + "前往 " + std::to_string(node.floor_num) + " 层");
            } else if (i == route.path_length - 1) {
                steps.push_back("沿走廊到达 " + node.name);
            } else {
                steps.push_back("前往 " + node.name);
            }

            const auto& neighbors = graph.get_neighbors(route.path[i - 1]);
            for (int j = 0; j < neighbors.size(); j++) {
                if (neighbors[j].to == route.path[i]) {
                    total_distance += neighbors[j].distance;
                    total_time += neighbors[j].time;
                    break;
                }
            }
        }

        return {
            {"found", true},
            {"building_id", building_id},
            {"path", path},
            {"path_names", path_names},
            {"floors", floors},
            {"steps", steps},
            {"floor_changes", route.floor_changes},
            {"distance", total_distance},
            {"time", total_time},
            {"strategy", strategy_text}
        };
    }
};

} // namespace service
