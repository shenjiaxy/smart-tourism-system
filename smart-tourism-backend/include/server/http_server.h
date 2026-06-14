#ifndef SMART_TOURISM_HTTP_SERVER_H
#define SMART_TOURISM_HTTP_SERVER_H

#include "httplib.h"
#include <string>
#include <functional>

/**
 * HTTP 服务器管理类
 * 封装 cpp-httplib，提供路由注册和服务器生命周期管理
 */
class HttpServer {
public:
    HttpServer();

    /** 注册所有 API 路由 */
    void register_routes();

    /** 启动服务器（阻塞） */
    void run(const std::string& host = "0.0.0.0", int port = 8080);

    /** 停止服务器 */
    void stop();

    /** 设置前端静态文件目录（生产模式） */
    void set_static_dir(const std::string& dir);

    /** 获取 httplib::Server 引用（供路由注册使用） */
    httplib::Server& server() { return server_; }

private:
    httplib::Server server_;
    std::string static_dir_;

    // ========== 路由注册方法 ==========

    // --- 景点推荐 ---
    void register_spot_routes();

    // --- 路线规划 ---
    void register_route_routes();

    // --- 场所查询 ---
    void register_facility_routes();

    // --- 旅游日记 ---
    void register_diary_routes();

    // --- 美食推荐 ---
    void register_food_routes();

    // --- AIGC 图生视频 ---
    void register_aigc_routes();

    // --- 通用路由 ---
    void register_common_routes();

    // --- 登录与会话 ---
    void register_auth_routes();

    // --- 管理员后台 ---
    void register_admin_routes();

    // ========== CORS 中间件 ==========
    void setup_cors();
};

#endif // SMART_TOURISM_HTTP_SERVER_H
