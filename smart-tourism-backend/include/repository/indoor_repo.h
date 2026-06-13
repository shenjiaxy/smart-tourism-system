#pragma once

#include "json.hpp"
#include "repository/db_connection.h"
#include <sstream>

using json = nlohmann::json;

namespace repository {

class IndoorRepo {
public:
    static json get_buildings(int area_id) {
        json result = json::array();
        if (area_id <= 0) return result;

        std::ostringstream sql;
        sql << "SELECT b.id, b.area_id, b.name, b.type, b.total_floors, "
            << "b.has_elevator, b.description, COUNT(n.id) "
            << "FROM buildings b JOIN indoor_nodes n ON n.building_id = b.id "
            << "WHERE b.area_id = " << area_id << " GROUP BY b.id ORDER BY b.name";
        Database::get().query(sql.str(), [&result](int, char** values, char**) {
            result.push_back({
                {"id", values[0] ? std::stoi(values[0]) : 0},
                {"area_id", values[1] ? std::stoi(values[1]) : 0},
                {"name", values[2] ? values[2] : ""},
                {"type", values[3] ? values[3] : ""},
                {"total_floors", values[4] ? std::stoi(values[4]) : 1},
                {"has_elevator", values[5] ? std::stoi(values[5]) : 0},
                {"description", values[6] ? values[6] : ""},
                {"node_count", values[7] ? std::stoi(values[7]) : 0}
            });
            return true;
        });
        return result;
    }

    static json get_nodes(int building_id) {
        json result = json::array();
        std::ostringstream sql;
        sql << "SELECT id, building_id, floor, name, type, pos_x, pos_y "
            << "FROM indoor_nodes WHERE building_id = " << building_id
            << " ORDER BY floor, id";
        Database::get().query(sql.str(), [&result](int, char** values, char**) {
            result.push_back({
                {"id", values[0] ? std::stoi(values[0]) : 0},
                {"building_id", values[1] ? std::stoi(values[1]) : 0},
                {"floor", values[2] ? std::stoi(values[2]) : 1},
                {"name", values[3] ? values[3] : ""},
                {"type", values[4] ? values[4] : ""},
                {"pos_x", values[5] ? std::stod(values[5]) : 0.0},
                {"pos_y", values[6] ? std::stod(values[6]) : 0.0}
            });
            return true;
        });
        return result;
    }

    static json get_roads(int building_id) {
        json result = json::array();
        std::ostringstream sql;
        sql << "SELECT id, building_id, from_node, to_node, distance "
            << "FROM indoor_roads WHERE building_id = " << building_id << " ORDER BY id";
        Database::get().query(sql.str(), [&result](int, char** values, char**) {
            result.push_back({
                {"id", values[0] ? std::stoi(values[0]) : 0},
                {"building_id", values[1] ? std::stoi(values[1]) : 0},
                {"from_node", values[2] ? std::stoi(values[2]) : 0},
                {"to_node", values[3] ? std::stoi(values[3]) : 0},
                {"distance", values[4] ? std::stod(values[4]) : 0.0}
            });
            return true;
        });
        return result;
    }
};

} // namespace repository
