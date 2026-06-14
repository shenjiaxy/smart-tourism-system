#pragma once
#include "json.hpp"
#include "repository/db_connection.h"
#include <string>
#include <sstream>

using json = nlohmann::json;

namespace repository {

/**
 * 美食数据访问层
 * 提供美食查询、搜索、名称列表获取等功能
 */
class FoodRepo {
public:
    /**
     * 获取美食列表（带排序分页和菜系过滤）
     * @param area_id 所属景区/校园ID
     * @param page 页码（从1开始）
     * @param page_size 每页数量
     * @param sort_by 排序字段: "rating" | "popularity" | "price"
     * @param cuisine 菜系过滤: ""=全部
     * @return 美食数组
     */
    static json get_all(int area_id, int page = 1, int page_size = 20,
                        const std::string& sort_by = "rating",
                        const std::string& cuisine = "") {
        json result = json::array();
        auto& db = Database::get();

        // 验证排序字段合法性
        std::string col;
        if (sort_by == "popularity" || sort_by == "price") col = sort_by;
        else col = "rating";

        std::ostringstream sql;
        sql << "SELECT f.id, f.area_id, f.name, f.cuisine, f.restaurant, f.rating, f.rating_count, "
            << "f.popularity, f.node_id, f.price, f.description, f.image, "
            << "COALESCE(n.pos_x, 0), COALESCE(n.pos_y, 0) "
            << "FROM foods f LEFT JOIN nodes n ON f.node_id = n.id "
            << "WHERE f.area_id = " << area_id;

        if (!cuisine.empty()) {
            sql << " AND f.cuisine = '" << escape_sql(cuisine) << "'";
        }

        sql << " ORDER BY " << col << " DESC";
        int offset = (page - 1) * page_size;
        sql << " LIMIT " << page_size << " OFFSET " << offset;

        db.query(sql.str(), [&result](int cols, char** values, char**) {
            json item;
            item["id"]          = values[0] ? std::stoi(values[0]) : 0;
            item["area_id"]     = values[1] ? std::stoi(values[1]) : 0;
            item["name"]        = values[2] ? values[2] : "";
            item["cuisine"]     = values[3] ? values[3] : "";
            item["restaurant"]  = values[4] ? values[4] : "";
            item["rating"]      = values[5] ? std::stod(values[5]) : 0.0;
            item["rating_count"]= values[6] ? std::stoi(values[6]) : 0;
            item["popularity"]  = values[7] ? std::stoi(values[7]) : 0;
            item["node_id"]     = values[8] ? std::stoi(values[8]) : 0;
            item["price"]       = values[9] ? std::stod(values[9]) : 0.0;
            item["description"] = values[10] ? values[10] : "";
            item["image"]       = values[11] ? values[11] : "";
            item["pos_x"]       = values[12] ? std::stod(values[12]) : 0.0;
            item["pos_y"]       = values[13] ? std::stod(values[13]) : 0.0;
            result.push_back(item);
            return true;
        });

        return result;
    }

    /**
     * 搜索美食（名称/饭店模糊匹配）
     * @param area_id 所属景区/校园ID
     * @param keyword 搜索关键字
     * @param limit 最大返回数量
     * @return 匹配的美食数组
     */
    static json search(int area_id, const std::string& keyword, int limit = 20) {
        json result = json::array();
        if (keyword.empty()) return result;

        auto& db = Database::get();
        std::string escaped = escape_sql(keyword);

        std::ostringstream sql;
        sql << "SELECT f.id, f.area_id, f.name, f.cuisine, f.restaurant, f.rating, f.rating_count, "
            << "f.popularity, f.node_id, f.price, f.description, f.image, "
            << "COALESCE(n.pos_x, 0), COALESCE(n.pos_y, 0) "
            << "FROM foods f LEFT JOIN nodes n ON f.node_id = n.id "
            << "WHERE f.area_id = " << area_id
            << " AND (f.name LIKE '%" << escaped << "%' "
            << "OR f.restaurant LIKE '%" << escaped << "%' "
            << "OR f.cuisine LIKE '%" << escaped << "%') "
            << "ORDER BY f.rating DESC LIMIT " << limit;

        int row_count = 0;
        db.query(sql.str(), [&result, &row_count](int cols, char** values, char**) {
            row_count++;
            json item;
            item["id"]          = values[0] ? std::stoi(values[0]) : 0;
            item["area_id"]     = values[1] ? std::stoi(values[1]) : 0;
            item["name"]        = values[2] ? values[2] : "";
            item["cuisine"]     = values[3] ? values[3] : "";
            item["restaurant"]  = values[4] ? values[4] : "";
            item["rating"]      = values[5] ? std::stod(values[5]) : 0.0;
            item["rating_count"]= values[6] ? std::stoi(values[6]) : 0;
            item["popularity"]  = values[7] ? std::stoi(values[7]) : 0;
            item["node_id"]     = values[8] ? std::stoi(values[8]) : 0;
            item["price"]       = values[9] ? std::stod(values[9]) : 0.0;
            item["description"] = values[10] ? values[10] : "";
            item["image"]       = values[11] ? values[11] : "";
            item["pos_x"]       = values[12] ? std::stod(values[12]) : 0.0;
            item["pos_y"]       = values[13] ? std::stod(values[13]) : 0.0;
            result.push_back(item);
            return true;
        });

        return result;
    }

    /**
     * 获取美食总数
     * @param area_id 所属景区/校园ID
     * @param cuisine 菜系过滤: ""=全部
     * @return 符合条件的美食总数
     */
    static int count(int area_id, const std::string& cuisine = "") {
        auto& db = Database::get();

        std::ostringstream sql;
        sql << "SELECT COUNT(*) FROM foods WHERE area_id = " << area_id;

        if (!cuisine.empty()) {
            sql << " AND cuisine = '" << escape_sql(cuisine) << "'";
        }

        return db.query_int(sql.str());
    }

private:
    /**
     * 简单SQL字符串转义（防注入）
     */
    static std::string escape_sql(const std::string& input) {
        std::string result;
        result.reserve(input.size() * 2);
        for (char c : input) {
            if (c == '\'') {
                result += "''";
            } else {
                result += c;
            }
        }
        return result;
    }
};

} // namespace repository
