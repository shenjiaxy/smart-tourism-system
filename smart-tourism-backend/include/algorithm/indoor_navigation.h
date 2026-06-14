#ifndef INDOOR_NAVIGATION_H
#define INDOOR_NAVIGATION_H

#include "dijkstra.h"

namespace algorithm {

struct IndoorPathResult {
    bool found;
    double total_distance;
    int floor_changes;
    int path_length;
    int* path;

    IndoorPathResult()
        : found(false), total_distance(0.0), floor_changes(0), path_length(0), path(nullptr) {}

    ~IndoorPathResult() { delete[] path; }

    IndoorPathResult(const IndoorPathResult&) = delete;
    IndoorPathResult& operator=(const IndoorPathResult&) = delete;

    IndoorPathResult(IndoorPathResult&& other) noexcept
        : found(other.found), total_distance(other.total_distance),
          floor_changes(other.floor_changes), path_length(other.path_length), path(other.path) {
        other.path = nullptr;
        other.path_length = 0;
    }
};

class IndoorNavigation {
public:
    static IndoorPathResult plan(const Graph& graph, int from_idx, int to_idx,
                                 DijkstraStrategy strategy = DijkstraStrategy::SHORTEST_DISTANCE) {
        IndoorPathResult result;
        if (from_idx < 0 || to_idx < 0 || from_idx >= graph.node_count() ||
            to_idx >= graph.node_count()) {
            return result;
        }

        DijkstraResult shortest = Dijkstra::run(graph, from_idx, strategy, 0x1);
        auto path = new int[graph.node_count() + 1];
        int length = shortest.trace_path(to_idx, path);
        if (length <= 0) {
            delete[] path;
            return result;
        }

        result.found = true;
        result.total_distance = shortest.dist[to_idx];
        result.path_length = length;
        result.path = path;
        for (int i = 1; i < length; i++) {
            if (graph.get_node(path[i - 1]).floor_num != graph.get_node(path[i]).floor_num) {
                result.floor_changes++;
            }
        }
        return result;
    }
};

} // namespace algorithm

#endif // INDOOR_NAVIGATION_H
