/**
 * @file food_service.h
 * @brief 美食推荐服务 —— Top-K堆排序推荐 + 编辑距离模糊搜索
 *
 * 核心算法：
 *   - Top-K 堆排序：从所有美食中选出评分/热度/距离最佳的K个（不完全排序）
 *   - 编辑距离（Levenshtein Distance）：对美食名/菜系/饭店进行模糊匹配
 *   - 相似度计算：1 - edit_distance / max(len1, len2)
 *
 * 使用场景：
 *   - "推荐附近美食" -> Top-K 排序推荐（热度/评价/距离 + 菜系过滤）
 *   - "我想吃火锅" -> 模糊匹配美食名/菜系/饭店
 *   - "找一家川菜馆" -> 按菜系过滤 + 模糊搜索
 */

#pragma once

#include "json.hpp"
#include "algorithm/heap.h"
#include "algorithm/edit_distance.h"
#include "repository/food_repo.h"
#include <string>
#include <memory>
#include <cmath>
#include <vector>
#include <algorithm>
#include <unordered_set>

using json = nlohmann::json;

namespace service {

class FoodService {
private:
    struct FoodScore {
        int id;
        int area_id;
        double score;
        std::string name;
        double rating;
        int rating_count;
        int popularity;
        double price;
        std::string cuisine;
        std::string restaurant;
        int node_id;
        std::string description;
        std::string image;
        double pos_x;
        double pos_y;
    };

    static double calc_distance(double x1, double y1, double x2, double y2) {
        return std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
    }

public:
    /**
     * @brief 获取美食推荐（Top-K 堆排序，不完全排序）
     *
     * @param area_id     区域ID
     * @param limit       返回数量（默认10）
     * @param sort_by     排序依据: "rating" | "popularity" | "distance" | "price_asc" | "price_desc"
     * @param cuisine     菜系过滤（空字符串表示不过滤）
     * @param ref_x       参考点X坐标（sort_by=distance 时使用）
     * @param ref_y       参考点Y坐标（sort_by=distance 时使用）
     * @return json 推荐美食列表
     */
    static json get_recommendations(int area_id, int limit = 10,
                                     const std::string& sort_by = "rating",
                                     const std::string& cuisine = "",
                                     double ref_x = -1, double ref_y = -1) {
        json result;
        try {
            if (area_id <= 0) {
                result["error"] = "无效的区域ID";
                return result;
            }

            json foods_json = repository::FoodRepo::get_all(area_id, 1, 10000, "rating", cuisine);

            if (!foods_json.is_array() || foods_json.empty()) {
                result["data"] = json::array();
                result["total"] = 0;
                return result;
            }

            int count = static_cast<int>(foods_json.size());

            auto foods = std::make_unique<FoodScore[]>(count);
            for (int i = 0; i < count; i++) {
                const auto& f = foods_json[i];
                foods[i].id = f.value("id", 0);
                foods[i].area_id = f.value("area_id", area_id);
                foods[i].name = f.value("name", "");
                foods[i].rating = f.value("rating", 0.0);
                foods[i].rating_count = f.value("rating_count", 0);
                foods[i].popularity = f.value("popularity", 0);
                foods[i].price = f.value("price", 0.0);
                foods[i].cuisine = f.value("cuisine", "");
                foods[i].restaurant = f.value("restaurant", "");
                foods[i].node_id = f.value("node_id", 0);
                foods[i].description = f.value("description", "");
                foods[i].image = f.value("image", "");
                foods[i].pos_x = f.value("pos_x", 0.0);
                foods[i].pos_y = f.value("pos_y", 0.0);

                if (sort_by == "distance" && ref_x >= 0 && ref_y >= 0) {
                    foods[i].score = calc_distance(ref_x, ref_y, foods[i].pos_x, foods[i].pos_y);
                } else if (sort_by == "price_asc") {
                    foods[i].score = -foods[i].price;
                } else if (sort_by == "price_desc") {
                    foods[i].score = foods[i].price;
                } else if (sort_by == "popularity") {
                    foods[i].score = static_cast<double>(foods[i].popularity);
                } else {
                    foods[i].score = foods[i].rating;
                }
            }

            // 使用 Top-K 堆排序取前 limit 个（不完全排序，O(n log k)）
            int k = (limit < count) ? limit : count;
            auto top_k = std::make_unique<FoodScore[]>(k);

            auto get_score = [](const FoodScore& f) -> double { return f.score; };

            // distance 和 price_asc 用升序（取最近的/最便宜的），其他用降序
            bool descending = (sort_by != "distance" && sort_by != "price_asc");
            int actual_k = algorithm::top_k_by_score(foods.get(), count, k, top_k.get(), get_score, descending);

            json items = json::array();
            for (int i = 0; i < actual_k; i++) {
                json item;
                item["id"] = top_k[i].id;
                item["area_id"] = top_k[i].area_id;
                item["name"] = top_k[i].name;
                item["rating"] = top_k[i].rating;
                item["rating_count"] = top_k[i].rating_count;
                item["popularity"] = top_k[i].popularity;
                item["price"] = top_k[i].price;
                item["cuisine"] = top_k[i].cuisine;
                item["restaurant"] = top_k[i].restaurant;
                item["node_id"] = top_k[i].node_id;
                item["description"] = top_k[i].description;
                item["image"] = top_k[i].image;
                item["pos_x"] = top_k[i].pos_x;
                item["pos_y"] = top_k[i].pos_y;

                if (sort_by == "distance" && ref_x >= 0 && ref_y >= 0) {
                    item["distance"] = top_k[i].score;
                }

                items.push_back(item);
            }

            int total = repository::FoodRepo::count(area_id, cuisine);

            result["data"] = items;
            result["total"] = total;
            result["limit"] = actual_k;
            if (sort_by == "distance" && ref_x >= 0 && ref_y >= 0) {
                result["ref_x"] = ref_x;
                result["ref_y"] = ref_y;
            }
        } catch (const std::exception& e) {
            result["error"] = std::string("美食推荐服务异常: ") + e.what();
        }
        return result;
    }

    /**
     * @brief 模糊搜索美食（编辑距离算法）
     *
     * 对美食名称、菜系、饭店名称进行基于编辑距离的模糊匹配。
     * 支持拼写错误和近似匹配，结果按相似度或指定字段排序。
     *
     * @param area_id  区域ID
     * @param keyword  搜索关键词
     * @param limit    返回数量
     * @param sort_by  排序依据: "similarity"(默认) | "rating" | "popularity" | "distance"
     * @param ref_x    参考点X坐标（sort_by=distance 时使用）
     * @param ref_y    参考点Y坐标（sort_by=distance 时使用）
     * @return json 搜索结果（含相似度分数）
     */
    static json search_foods(int area_id, const std::string& keyword, int limit = 20,
                              const std::string& sort_by = "similarity",
                              double ref_x = -1, double ref_y = -1) {
        json result;
        try {
            if (area_id <= 0) {
                result["error"] = "无效的区域ID";
                return result;
            }

            if (keyword.empty()) {
                result["error"] = "搜索关键词不能为空";
                return result;
            }

            // 1. 先用数据库LIKE精确搜索（名/菜系/饭店均可命中）
            json db_results = repository::FoodRepo::search(area_id, keyword, 200);

            // 2. 同时用编辑距离对名/菜系/饭店进行模糊匹配
            json all_foods = repository::FoodRepo::get_all(area_id, 1, 10000, "rating", "");

            struct MatchItem {
                json food;
                double similarity;
            };

            std::vector<MatchItem> matches;

            if (all_foods.is_array() && !all_foods.empty()) {
                int total_count = static_cast<int>(all_foods.size());

                // 收集所有可搜索文本（名称、菜系、饭店）
                auto texts = std::make_unique<std::string[]>(total_count);
                for (int i = 0; i < total_count; i++) {
                    std::string name = all_foods[i].value("name", "");
                    std::string cuisine = all_foods[i].value("cuisine", "");
                    std::string restaurant = all_foods[i].value("restaurant", "");
                    texts[i] = name + " " + cuisine + " " + restaurant;
                }

                int k = (limit * 2 < total_count) ? limit * 2 : total_count;
                auto result_indices = std::make_unique<int[]>(k);
                auto result_sims = std::make_unique<double[]>(k);

                int match_count = algorithm::EditDistance::fuzzy_match_top_k(
                    keyword, texts.get(), total_count,
                    result_indices.get(), result_sims.get(),
                    k, 0.25  // 相似度阈值放宽，因为搜索范围更广
                );

                // 去重（按 food id）
                std::unordered_set<int> seen_ids;
                for (auto& db_item : db_results) {
                    seen_ids.insert(db_item.value("id", -1));
                }

                for (int i = 0; i < match_count; i++) {
                    int idx = result_indices[i];
                    int fid = all_foods[idx].value("id", -1);
                    if (seen_ids.count(fid)) continue;  // DB已命中则跳过
                    seen_ids.insert(fid);
                    MatchItem m;
                    m.food = all_foods[idx];
                    m.similarity = result_sims[i];
                    matches.push_back(m);
                }
            }

            // 3. 合并结果：DB结果（精确匹配，相似度=1.0） + 编辑距离结果
            std::vector<MatchItem> all_matches;

            if (db_results.is_array()) {
                for (auto& item : db_results) {
                    MatchItem m;
                    m.food = item;
                    m.similarity = 1.0;
                    all_matches.push_back(m);
                }
            }

            for (auto& m : matches) {
                all_matches.push_back(m);
            }

            // 4. 按 sort_by 排序
            if (sort_by == "rating") {
                std::sort(all_matches.begin(), all_matches.end(),
                    [](const MatchItem& a, const MatchItem& b) {
                        return a.food.value("rating", 0.0) > b.food.value("rating", 0.0);
                    });
            } else if (sort_by == "popularity") {
                std::sort(all_matches.begin(), all_matches.end(),
                    [](const MatchItem& a, const MatchItem& b) {
                        return a.food.value("popularity", 0) > b.food.value("popularity", 0);
                    });
            } else if (sort_by == "distance" && ref_x >= 0 && ref_y >= 0) {
                std::sort(all_matches.begin(), all_matches.end(),
                    [ref_x, ref_y](const MatchItem& a, const MatchItem& b) {
                        double da = calc_distance(ref_x, ref_y,
                            a.food.value("pos_x", 0.0), a.food.value("pos_y", 0.0));
                        double db = calc_distance(ref_x, ref_y,
                            b.food.value("pos_x", 0.0), b.food.value("pos_y", 0.0));
                        return da < db;
                    });
            } else {
                // 默认：按相似度降序
                std::sort(all_matches.begin(), all_matches.end(),
                    [](const MatchItem& a, const MatchItem& b) {
                        return a.similarity > b.similarity;
                    });
            }

            // 5. 截取 limit 条
            int result_count = std::min(limit, static_cast<int>(all_matches.size()));

            json items = json::array();
            for (int i = 0; i < result_count; i++) {
                json item = all_matches[i].food;
                item["similarity"] = all_matches[i].similarity;

                if (sort_by == "distance" && ref_x >= 0 && ref_y >= 0) {
                    double d = calc_distance(ref_x, ref_y,
                        all_matches[i].food.value("pos_x", 0.0),
                        all_matches[i].food.value("pos_y", 0.0));
                    item["distance"] = d;
                }

                items.push_back(item);
            }

            result["data"] = items;
            result["total"] = static_cast<int>(all_matches.size());
            result["mode"] = "fuzzy";
            result["sort_by"] = sort_by;
        } catch (const std::exception& e) {
            result["error"] = std::string("美食搜索服务异常: ") + e.what();
        }
        return result;
    }
};

} // namespace service
