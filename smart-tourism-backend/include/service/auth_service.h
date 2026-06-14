#ifndef SMART_TOURISM_AUTH_SERVICE_H
#define SMART_TOURISM_AUTH_SERVICE_H

#include "repository/db_connection.h"
#include "json.hpp"

#include <iomanip>
#include <cstdint>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

namespace service {

struct AuthUser {
    int id = 0;
    std::string username;
    std::string nickname;
    std::string avatar;
    std::string role;

    nlohmann::json to_json() const {
        return {
            {"id", id},
            {"username", username},
            {"nickname", nickname},
            {"avatar", avatar},
            {"role", role}
        };
    }
};

struct LoginResult {
    bool success = false;
    std::string token;
    AuthUser user;
};

class AuthService {
public:
    static LoginResult login(const std::string& username,
                             const std::string& password) {
        LoginResult result;
        if (username.empty() || password.empty()) return result;

        auto& db = Database::get();
        sqlite3_stmt* stmt = nullptr;
        const char* sql =
            "SELECT id, username, nickname, avatar, role "
            "FROM users WHERE username = ? AND password = ? LIMIT 1";

        if (sqlite3_prepare_v2(db.get_raw(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return result;
        }

        struct StatementGuard {
            sqlite3_stmt* stmt;
            ~StatementGuard() { sqlite3_finalize(stmt); }
        } guard{stmt};

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_ROW) return result;

        result.user.id = sqlite3_column_int(stmt, 0);
        result.user.username = column_text(stmt, 1);
        result.user.nickname = column_text(stmt, 2);
        result.user.avatar = column_text(stmt, 3);
        result.user.role = column_text(stmt, 4);
        result.token = generate_token();
        result.success = true;

        std::lock_guard<std::mutex> lock(session_mutex_);
        sessions_[result.token] = result.user.id;
        return result;
    }

    static std::optional<AuthUser> find_session(const std::string& token) {
        if (token.empty()) return std::nullopt;
        int user_id = 0;
        {
            std::lock_guard<std::mutex> lock(session_mutex_);
            auto it = sessions_.find(token);
            if (it == sessions_.end()) return std::nullopt;
            user_id = it->second;
        }
        return find_user_by_id(user_id);
    }

    static void logout(const std::string& token) {
        std::lock_guard<std::mutex> lock(session_mutex_);
        sessions_.erase(token);
    }

    static bool is_admin(const AuthUser& user) {
        return user.role == "admin";
    }

private:
    static std::string column_text(sqlite3_stmt* stmt, int column) {
        const auto* value = sqlite3_column_text(stmt, column);
        return value ? reinterpret_cast<const char*>(value) : "";
    }

    static std::optional<AuthUser> find_user_by_id(int user_id) {
        auto& db = Database::get();
        sqlite3_stmt* stmt = nullptr;
        const char* sql =
            "SELECT id, username, nickname, avatar, role "
            "FROM users WHERE id = ? LIMIT 1";
        if (sqlite3_prepare_v2(db.get_raw(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return std::nullopt;
        }

        struct StatementGuard {
            sqlite3_stmt* stmt;
            ~StatementGuard() { sqlite3_finalize(stmt); }
        } guard{stmt};

        sqlite3_bind_int(stmt, 1, user_id);
        if (sqlite3_step(stmt) != SQLITE_ROW) return std::nullopt;

        AuthUser user;
        user.id = sqlite3_column_int(stmt, 0);
        user.username = column_text(stmt, 1);
        user.nickname = column_text(stmt, 2);
        user.avatar = column_text(stmt, 3);
        user.role = column_text(stmt, 4);
        return user;
    }

    static std::string generate_token() {
        static std::mutex generator_mutex;
        static std::mt19937_64 generator(std::random_device{}());
        static std::uint64_t sequence = 0;
        std::lock_guard<std::mutex> lock(generator_mutex);
        std::ostringstream token;
        token << std::hex << std::setfill('0');
        for (int i = 0; i < 4; ++i) {
            token << std::setw(16) << generator();
        }
        token << std::setw(16) << ++sequence;
        return token.str();
    }

    inline static std::unordered_map<std::string, int> sessions_;
    inline static std::mutex session_mutex_;
};

} // namespace service

#endif // SMART_TOURISM_AUTH_SERVICE_H
