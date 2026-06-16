#pragma once
#include "json.hpp"
#include "repository/db_connection.h"
#include <string>
#include <sstream>
#include <ctime>

using json = nlohmann::json;

namespace repository {

/**
 * 旅游日记数据访问层
 * 提供日记的CRUD、压缩内容管理、评分、搜索等功能
 */
class DiaryRepo {
public:
    /**
     * 获取日记列表（带排序分页）
     * @param page 页码（从1开始）
     * @param page_size 每页数量
     * @param sort_by 排序字段: "popularity" | "rating" | "created_at"
     * @param order 排序方向: "desc" | "asc"
     * @param destination_id 目的地景区ID过滤（-1=全部）
     * @return 日记数组
     */
    static json get_all(int page = 1, int page_size = 20,
                        const std::string& sort_by = "created_at",
                        const std::string& order = "desc",
                        int destination_id = -1) {
        json result = json::array();
        auto& db = Database::get();

        // 验证排序字段合法性
        std::string col;
        if (sort_by == "popularity") col = "popularity";
        else if (sort_by == "rating") col = "avg_rating";
        else col = "created_at";

        std::string dir = (order == "asc") ? "ASC" : "DESC";

        std::ostringstream sql;
        sql << "SELECT d.id, d.user_id, u.username, u.nickname, "
            << "d.title, d.content, d.is_compressed, "
            << "d.destination, d.destination_id, d.tags, d.images, d.videos, "
            << "d.popularity, d.avg_rating, d.rating_count, "
            << "d.created_at, d.updated_at "
            << "FROM diaries d "
            << "LEFT JOIN users u ON d.user_id = u.id";

        if (destination_id > 0) {
            sql << " WHERE d.destination_id = " << destination_id;
        }

        sql << " ORDER BY d." << col << " " << dir;
        int offset = (page - 1) * page_size;
        sql << " LIMIT " << page_size << " OFFSET " << offset;

        db.query(sql.str(), [&result](int cols, char** values, char**) {
            json item;
            item["id"]              = values[0] ? std::stoi(values[0]) : 0;
            item["user_id"]         = values[1] ? std::stoi(values[1]) : 0;
            item["username"]        = values[2] ? values[2] : "";
            item["nickname"]        = values[3] ? values[3] : "";
            item["title"]           = values[4] ? values[4] : "";
            item["content"]         = values[5] ? values[5] : "";
            item["is_compressed"]   = values[6] ? (std::stoi(values[6]) != 0) : false;
            item["destination"]     = values[7] ? values[7] : "";
            item["destination_id"]  = values[8] ? std::stoi(values[8]) : 0;
            item["tags"]            = values[9]  ? values[9]  : "[]";
            item["images"]          = values[10] ? values[10] : "[]";
            item["videos"]          = values[11] ? values[11] : "[]";
            item["popularity"]      = values[12] ? std::stoi(values[12]) : 0;
            item["avg_rating"]      = values[13] ? std::stod(values[13]) : 0.0;
            item["rating_count"]    = values[14] ? std::stoi(values[14]) : 0;
            item["created_at"]      = values[15] ? values[15] : "";
            item["updated_at"]      = values[16] ? values[16] : "";
            result.push_back(item);
            return true;
        });

        return result;
    }

    /**
     * 获取单个日记详情
     * @param id 日记ID
     * @return 日记对象，未找到时返回空对象
     */
    static json get_by_id(int id) {
        json result;
        auto& db = Database::get();

        std::ostringstream sql;
        sql << "SELECT d.id, d.user_id, u.username, u.nickname, "
            << "d.title, d.content, d.compressed_content, d.is_compressed, "
            << "d.destination, d.destination_id, d.tags, d.images, d.videos, "
            << "d.popularity, d.avg_rating, d.rating_count, "
            << "d.created_at, d.updated_at "
            << "FROM diaries d "
            << "LEFT JOIN users u ON d.user_id = u.id "
            << "WHERE d.id = " << id << " LIMIT 1";

        bool found = false;
        db.query(sql.str(), [&result, &found](int cols, char** values, char**) {
            found = true;
            result["id"]              = values[0] ? std::stoi(values[0]) : 0;
            result["user_id"]         = values[1] ? std::stoi(values[1]) : 0;
            result["username"]        = values[2] ? values[2] : "";
            result["nickname"]        = values[3] ? values[3] : "";
            result["title"]           = values[4] ? values[4] : "";
            result["content"]         = values[5] ? values[5] : "";
            result["compressed_content"] = values[6] ? values[6] : "";
            result["is_compressed"]   = values[7] ? (std::stoi(values[7]) != 0) : false;
            result["destination"]     = values[8] ? values[8] : "";
            result["destination_id"]  = values[9] ? std::stoi(values[9]) : 0;
            result["tags"]            = values[10] ? values[10] : "[]";
            result["images"]          = values[11] ? values[11] : "[]";
            result["videos"]          = values[12] ? values[12] : "[]";
            result["popularity"]      = values[13] ? std::stoi(values[13]) : 0;
            result["avg_rating"]      = values[14] ? std::stod(values[14]) : 0.0;
            result["rating_count"]    = values[15] ? std::stoi(values[15]) : 0;
            result["created_at"]      = values[16] ? values[16] : "";
            result["updated_at"]      = values[17] ? values[17] : "";
            return false;
        });

        return found ? result : json();
    }

    /**
     * 创建日记
     * @param user_id 作者用户ID
     * @param title 标题
     * @param content 正文内容
     * @param destination 目的地名称
     * @param destination_id 目的地景区ID
     * @param tags 标签（JSON数组字符串）
     * @param images 图片路径（JSON数组字符串）
     * @param videos 视频路径（JSON数组字符串）
     * @return 新创建的日记ID
     */
    static int create(int user_id, const std::string& title, const std::string& content,
                      const std::string& destination = "", int destination_id = -1,
                      const std::string& tags = "[]", const std::string& images = "[]",
                      const std::string& videos = "[]") {
        auto& db = Database::get();

        std::ostringstream sql;
        sql << "INSERT INTO diaries (user_id, title, content, destination, destination_id, tags, images, videos) "
            << "VALUES (" << user_id << ", "
            << "'" << escape_sql(title) << "', "
            << "'" << escape_sql(content) << "', "
            << "'" << escape_sql(destination) << "', ";

        if (destination_id > 0) {
            sql << destination_id << ", ";
        } else {
            sql << "NULL, ";
        }

        sql << "'" << escape_sql(tags) << "', "
            << "'" << escape_sql(images) << "', "
            << "'" << escape_sql(videos) << "')";

        db.execute(sql.str());
        return static_cast<int>(db.last_insert_id());
    }

    /**
     * 更新日记（动态字段更新）
     * @param id 日记ID
     * @param fields 要更新的字段和值，如 {"title": "新标题", "content": "新内容"}
     * @return 是否更新成功
     */
    static bool update(int id, const json& fields) {
        if (!fields.is_object() || fields.empty()) return false;

        auto& db = Database::get();

        // 字段名到数据库列名的映射验证（白名单，防止SQL注入）
        static const std::vector<std::string> allowed_fields = {
            "title", "content", "compressed_content", "is_compressed",
            "destination", "destination_id", "tags", "images", "videos",
            "popularity", "avg_rating", "rating_count"
        };

        std::ostringstream sql;
        sql << "UPDATE diaries SET ";
        bool first = true;

        for (auto& [key, value] : fields.items()) {
            // 检查字段名是否在白名单中
            bool valid = false;
            for (const auto& af : allowed_fields) {
                if (key == af) { valid = true; break; }
            }
            if (!valid) continue;

            if (!first) sql << ", ";
            first = false;

            if (value.is_null()) {
                sql << key << " = NULL";
            } else if (value.is_number()) {
                sql << key << " = " << value.get<double>();
            } else if (value.is_boolean()) {
                sql << key << " = " << (value.get<bool>() ? 1 : 0);
            } else if (value.is_array() || value.is_object()) {
                sql << key << " = '" << escape_sql(value.dump()) << "'";
            } else {
                sql << key << " = '" << escape_sql(value.get<std::string>()) << "'";
            }
        }

        if (first) return false; // 没有有效字段被更新

        sql << ", updated_at = datetime('now', 'localtime') WHERE id = " << id;
        return db.execute(sql.str());
    }

    /**
     * 删除日记
     * @param id 日记ID
     * @return 是否删除成功
     */
    static bool remove(int id) {
        auto& db = Database::get();
        std::ostringstream sql;
        sql << "DELETE FROM diaries WHERE id = " << id;
        return db.execute(sql.str());
    }

    /**
     * 保存Huffman压缩后的内容（Base64编码）
     * @param id 日记ID
     * @param compressed_base64 Base64编码的压缩数据
     * @return 是否保存成功
     */
    static bool save_compressed(int id, const std::string& compressed_base64) {
        auto& db = Database::get();
        std::ostringstream sql;
        sql << "UPDATE diaries SET compressed_content = '" << escape_sql(compressed_base64) << "', "
            << "is_compressed = 1, "
            << "updated_at = datetime('now', 'localtime') "
            << "WHERE id = " << id;
        return db.execute(sql.str());
    }

    static bool clear_compressed(int id) {
        auto& db = Database::get();
        std::ostringstream sql;
        sql << "UPDATE diaries SET compressed_content = '', "
            << "is_compressed = 0, "
            << "updated_at = datetime('now', 'localtime') "
            << "WHERE id = " << id;
        return db.execute(sql.str());
    }

    /**
     * 获取压缩后的内容
     * @param id 日记ID
     * @return Base64编码的压缩数据，空字符串表示未找到
     */
    static std::string get_compressed(int id) {
        auto& db = Database::get();
        std::ostringstream sql;
        sql << "SELECT compressed_content FROM diaries WHERE id = " << id << " AND is_compressed = 1 LIMIT 1";
        return db.query_string(sql.str());
    }

    /**
     * 按标题精确查找日记
     * @param title 日记标题
     * @return 匹配的日记数组
     */
    static json find_by_title(const std::string& title) {
        json result = json::array();
        if (title.empty()) return result;

        auto& db = Database::get();
        std::ostringstream sql;
        sql << "SELECT d.id, d.user_id, u.username, u.nickname, "
            << "d.title, d.content, d.is_compressed, "
            << "d.destination, d.destination_id, d.tags, d.images, d.videos, "
            << "d.popularity, d.avg_rating, d.rating_count, "
            << "d.created_at, d.updated_at "
            << "FROM diaries d "
            << "LEFT JOIN users u ON d.user_id = u.id "
            << "WHERE d.title = '" << escape_sql(title) << "'";

        db.query(sql.str(), [&result](int cols, char** values, char**) {
            json item;
            item["id"]              = values[0] ? std::stoi(values[0]) : 0;
            item["user_id"]         = values[1] ? std::stoi(values[1]) : 0;
            item["username"]        = values[2] ? values[2] : "";
            item["nickname"]        = values[3] ? values[3] : "";
            item["title"]           = values[4] ? values[4] : "";
            item["content"]         = values[5] ? values[5] : "";
            item["is_compressed"]   = values[6] ? (std::stoi(values[6]) != 0) : false;
            item["destination"]     = values[7] ? values[7] : "";
            item["destination_id"]  = values[8] ? std::stoi(values[8]) : 0;
            item["tags"]            = values[9]  ? values[9]  : "[]";
            item["images"]          = values[10] ? values[10] : "[]";
            item["videos"]          = values[11] ? values[11] : "[]";
            item["popularity"]      = values[12] ? std::stoi(values[12]) : 0;
            item["avg_rating"]      = values[13] ? std::stod(values[13]) : 0.0;
            item["rating_count"]    = values[14] ? std::stoi(values[14]) : 0;
            item["created_at"]      = values[15] ? values[15] : "";
            item["updated_at"]      = values[16] ? values[16] : "";
            result.push_back(item);
            return true;
        });

        return result;
    }

    /**
     * 获取所有日记（用于全文索引构建）
     * 只返回ID、标题和内容字段，减少内存占用
     * @return 日记数组，每个元素: {id, title, content}
     */
    static json get_all_for_indexing() {
        json result = json::array();
        auto& db = Database::get();

        const std::string sql =
            "SELECT id, title, content FROM diaries WHERE content IS NOT NULL AND content != ''";

        db.query(sql, [&result](int cols, char** values, char**) {
            json item;
            item["id"]      = values[0] ? std::stoi(values[0]) : 0;
            item["title"]   = values[1] ? values[1] : "";
            item["content"] = values[2] ? values[2] : "";
            result.push_back(item);
            return true;
        });

        return result;
    }

    /**
     * 添加/更新评分
     * 如果用户已评分则更新，否则插入新评分
     * 同时更新日记的 avg_rating 和 rating_count
     * @param user_id 用户ID
     * @param diary_id 日记ID
     * @param score 评分（1-5）
     * @return 是否成功
     */
    static bool rate(int user_id, int diary_id, int score) {
        auto& db = Database::get();

        // 检查是否已评分
        std::ostringstream check_sql;
        check_sql << "SELECT score FROM ratings WHERE user_id = " << user_id
                  << " AND diary_id = " << diary_id << " LIMIT 1";

        int existing_score = db.query_int(check_sql.str(), -1);

        bool success;
        if (existing_score >= 0) {
            // 更新已有评分
            std::ostringstream update_sql;
            update_sql << "UPDATE ratings SET score = " << score
                       << " WHERE user_id = " << user_id
                       << " AND diary_id = " << diary_id;
            success = db.execute(update_sql.str());
        } else {
            // 插入新评分
            std::ostringstream insert_sql;
            insert_sql << "INSERT INTO ratings (user_id, diary_id, score) VALUES ("
                       << user_id << ", " << diary_id << ", " << score << ")";
            success = db.execute(insert_sql.str());
        }

        if (success) {
            // 重新计算平均评分
            std::ostringstream calc_sql;
            calc_sql << "UPDATE diaries SET "
                     << "avg_rating = (SELECT AVG(score) FROM ratings WHERE diary_id = " << diary_id << "), "
                     << "rating_count = (SELECT COUNT(*) FROM ratings WHERE diary_id = " << diary_id << "), "
                     << "updated_at = datetime('now', 'localtime') "
                     << "WHERE id = " << diary_id;
            db.execute(calc_sql.str());
        }

        return success;
    }

    /**
     * 增加日记浏览量（+1）
     * @param diary_id 日记ID
     */
    static void increment_popularity(int diary_id) {
        auto& db = Database::get();
        std::ostringstream sql;
        sql << "UPDATE diaries SET popularity = popularity + 1 WHERE id = " << diary_id;
        db.execute(sql.str());
    }

    /**
     * 获取日记总数
     * @param destination_id 目的地景区ID过滤（-1=全部）
     * @return 符合条件的日记总数
     */
    static int count(int destination_id = -1) {
        auto& db = Database::get();

        if (destination_id > 0) {
            std::ostringstream sql;
            sql << "SELECT COUNT(*) FROM diaries WHERE destination_id = " << destination_id;
            return db.query_int(sql.str());
        }

        return db.query_int("SELECT COUNT(*) FROM diaries");
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
