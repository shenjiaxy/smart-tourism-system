#ifndef SMART_TOURISM_ADMIN_SERVICE_H
#define SMART_TOURISM_ADMIN_SERVICE_H

#include "repository/db_connection.h"
#include "json.hpp"

#include <string>

namespace service {

class AdminService {
public:
    static nlohmann::json get_overview() {
        auto& db = Database::get();
        nlohmann::json data;
        data["users"] = db.query_int("SELECT COUNT(*) FROM users");
        data["admins"] = db.query_int("SELECT COUNT(*) FROM users WHERE role='admin'");
        data["scenic_spots"] = db.query_int("SELECT COUNT(*) FROM scenic_spots");
        data["nodes"] = db.query_int("SELECT COUNT(*) FROM nodes");
        data["roads"] = db.query_int("SELECT COUNT(*) FROM roads");
        data["diaries"] = db.query_int("SELECT COUNT(*) FROM diaries");
        data["foods"] = db.query_int("SELECT COUNT(*) FROM foods");
        data["ratings"] = db.query_int("SELECT COUNT(*) FROM ratings");
        return data;
    }

    static nlohmann::json get_users() {
        nlohmann::json users = nlohmann::json::array();
        Database::get().query(
            "SELECT id, username, nickname, avatar, role, created_at "
            "FROM users ORDER BY id",
            [&users](int, char** values, char**) {
                users.push_back({
                    {"id", to_int(values[0])},
                    {"username", to_string(values[1])},
                    {"nickname", to_string(values[2])},
                    {"avatar", to_string(values[3])},
                    {"role", to_string(values[4])},
                    {"created_at", to_string(values[5])}
                });
                return true;
            });
        return users;
    }

    static bool update_user_role(int user_id, const std::string& role) {
        if (user_id <= 0 || (role != "user" && role != "admin")) return false;

        auto& db = Database::get();
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "UPDATE users SET role = ? WHERE id = ?";
        if (sqlite3_prepare_v2(db.get_raw(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        StatementGuard guard{stmt};
        sqlite3_bind_text(stmt, 1, role.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, user_id);
        return sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db.get_raw()) > 0;
    }

    static nlohmann::json get_diaries() {
        nlohmann::json diaries = nlohmann::json::array();
        Database::get().query(
            "SELECT d.id, d.title, d.destination, d.popularity, "
            "d.avg_rating, d.rating_count, d.created_at, "
            "u.id, u.username, u.nickname "
            "FROM diaries d JOIN users u ON u.id = d.user_id "
            "ORDER BY d.created_at DESC, d.id DESC",
            [&diaries](int, char** values, char**) {
                diaries.push_back({
                    {"id", to_int(values[0])},
                    {"title", to_string(values[1])},
                    {"destination", to_string(values[2])},
                    {"popularity", to_int(values[3])},
                    {"avg_rating", to_double(values[4])},
                    {"rating_count", to_int(values[5])},
                    {"created_at", to_string(values[6])},
                    {"user_id", to_int(values[7])},
                    {"username", to_string(values[8])},
                    {"nickname", to_string(values[9])}
                });
                return true;
            });
        return diaries;
    }

    static bool delete_diary(int diary_id) {
        if (diary_id <= 0) return false;

        auto& db = Database::get();
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "DELETE FROM diaries WHERE id = ?";
        if (sqlite3_prepare_v2(db.get_raw(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        StatementGuard guard{stmt};
        sqlite3_bind_int(stmt, 1, diary_id);
        return sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db.get_raw()) > 0;
    }

private:
    struct StatementGuard {
        sqlite3_stmt* stmt;
        ~StatementGuard() { sqlite3_finalize(stmt); }
    };

    static std::string to_string(const char* value) {
        return value ? value : "";
    }

    static int to_int(const char* value) {
        return value ? std::stoi(value) : 0;
    }

    static double to_double(const char* value) {
        return value ? std::stod(value) : 0.0;
    }
};

} // namespace service

#endif // SMART_TOURISM_ADMIN_SERVICE_H
