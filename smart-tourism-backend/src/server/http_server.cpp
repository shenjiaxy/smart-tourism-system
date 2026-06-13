#include "server/http_server.h"
#include "server/response.h"
#include "repository/db_connection.h"
#include "repository/spot_repo.h"
#include "service/recommend_service.h"
#include "service/route_service.h"
#include "service/indoor_route_service.h"
#include "service/facility_service.h"
#include "service/diary_service.h"
#include "service/food_service.h"
#include <iostream>

using json = nlohmann::json;

// ============================================================
// 构造函数
// ============================================================
HttpServer::HttpServer() {}

void HttpServer::register_routes() {
    setup_cors();
    register_common_routes();
    register_spot_routes();
    register_route_routes();
    register_facility_routes();
    register_diary_routes();
    register_food_routes();
    std::cout << "[Server] All routes registered." << std::endl;
}

void HttpServer::setup_cors() {
    server_.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
    });
    server_.Options("(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });
    std::cout << "[Server] CORS middleware configured." << std::endl;
}

void HttpServer::run(const std::string& host, int port) {
    if (!static_dir_.empty()) {
        server_.set_mount_point("/", static_dir_);
        std::cout << "[Server] Static files mounted from: " << static_dir_ << std::endl;
    }
    if (!server_.listen(host, port)) {
        std::cerr << "[Server] Failed to listen on " << host << ":" << port << std::endl;
    }
}

void HttpServer::stop() { server_.stop(); }
void HttpServer::set_static_dir(const std::string& dir) { static_dir_ = dir; }

// ============================================================
// 通用路由
// ============================================================
void HttpServer::register_common_routes() {
    server_.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(Response::ok("Server is running").dump(), "application/json");
    });

    server_.Get("/api",[](const httplib::Request&, httplib::Response& res) {
        json info;
        info["name"] = "Smart Tourism System API";
        info["version"] = "1.0.0";
        info["modules"] = {
            {{"name", "景点推荐"}, {"prefix", "/api/spots"}},
            {{"name", "路线规划"}, {"prefix", "/api/route"}},
            {{"name", "场所查询"}, {"prefix", "/api/facilities"}},
            {{"name", "旅游日记"}, {"prefix", "/api/diaries"}},
            {{"name", "美食推荐"}, {"prefix", "/api/foods"}}
        };
        res.set_content(Response::ok(info).dump(), "application/json");
    });
}

// ============================================================
// 景点推荐路由
// ============================================================
void HttpServer::register_spot_routes() {
    // GET /api/spots/options
    server_.Get("/api/spots/options", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string type = req.get_param_value("type");
            json result = repository::SpotRepo::get_all(1, 10000, "popularity", "desc", type, "");
            json options = json::array();
            for (const auto& spot : result) {
                json item;
                item["id"] = spot.value("id", 0);
                item["name"] = spot.value("name", "");
                item["type"] = spot.value("type", "");
                item["category"] = spot.value("category", "");
                item["city"] = spot.value("city", "");
                item["rating"] = spot.value("rating", 0.0);
                item["popularity"] = spot.value("popularity", 0);
                options.push_back(item);
            }
            res.set_content(Response::ok(options).dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    server_.Get("/api/stats/overview", [](const httplib::Request&, httplib::Response& res) {
        try {
            auto& db = Database::get();
            json stats;
            stats["database_status"] = db.is_open() ? "connected" : "closed";
            stats["scenic_spots"] = db.query_int("SELECT COUNT(*) FROM scenic_spots");
            stats["campus_count"] = db.query_int("SELECT COUNT(*) FROM scenic_spots WHERE type='campus'");
            stats["scenic_count"] = db.query_int("SELECT COUNT(*) FROM scenic_spots WHERE type='scenic'");
            stats["nodes"] = db.query_int("SELECT COUNT(*) FROM nodes");
            stats["buildings"] = db.query_int("SELECT COUNT(*) FROM nodes WHERE type='building'");
            stats["facilities"] = db.query_int("SELECT COUNT(*) FROM nodes WHERE type='facility'");
            stats["facility_types"] = db.query_int("SELECT COUNT(DISTINCT sub_type) FROM nodes WHERE type='facility'");
            stats["roads"] = db.query_int("SELECT COUNT(*) FROM roads");
            stats["users"] = db.query_int("SELECT COUNT(*) FROM users");
            stats["diaries"] = db.query_int("SELECT COUNT(*) FROM diaries");
            stats["foods"] = db.query_int("SELECT COUNT(*) FROM foods");
            stats["cuisines"] = db.query_int("SELECT COUNT(DISTINCT cuisine) FROM foods");
            stats["ratings"] = db.query_int("SELECT COUNT(*) FROM ratings");

            json requirements = json::array({
                {{"label", "scenic/campus count >= 200"}, {"actual", stats["scenic_spots"]}, {"target", 200}, {"passed", stats["scenic_spots"].get<int>() >= 200}},
                {{"label", "node count >= 4000"}, {"actual", stats["nodes"]}, {"target", 4000}, {"passed", stats["nodes"].get<int>() >= 4000}},
                {{"label", "facility count >= 50"}, {"actual", stats["facilities"]}, {"target", 50}, {"passed", stats["facilities"].get<int>() >= 50}},
                {{"label", "facility type count >= 10"}, {"actual", stats["facility_types"]}, {"target", 10}, {"passed", stats["facility_types"].get<int>() >= 10}},
                {{"label", "road edge count >= 200"}, {"actual", stats["roads"]}, {"target", 200}, {"passed", stats["roads"].get<int>() >= 200}},
                {{"label", "user count >= 10"}, {"actual", stats["users"]}, {"target", 10}, {"passed", stats["users"].get<int>() >= 10}}
            });

            json hot_spots = json::array();
            db.query("SELECT id, name, type, category, popularity, rating FROM scenic_spots ORDER BY popularity DESC LIMIT 8",
                [&hot_spots](int, char** vals, char**) {
                    json item;
                    item["id"] = std::stoi(vals[0]);
                    item["name"] = vals[1] ? vals[1] : "";
                    item["type"] = vals[2] ? vals[2] : "";
                    item["category"] = vals[3] ? vals[3] : "";
                    item["popularity"] = std::stoi(vals[4]);
                    item["rating"] = std::stod(vals[5]);
                    hot_spots.push_back(item);
                    return true;
                });

            json data;
            data["stats"] = stats;
            data["requirements"] = requirements;
            data["hot_spots"] = hot_spots;
            res.set_content(Response::ok(data).dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // GET /api/spots/recommend
    server_.Get("/api/spots/recommend", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string sort_by = req.get_param_value("sort_by");
            if (sort_by.empty()) sort_by = "popularity";
            std::string limit_str = req.get_param_value("limit");
            int limit = limit_str.empty() ? 10 : std::stoi(limit_str);
            std::string user_id_str = req.get_param_value("user_id");
            int user_id = user_id_str.empty() ? -1 : std::stoi(user_id_str);
            std::string type = req.get_param_value("type");
            std::string category = req.get_param_value("category");

            json result = service::RecommendService::get_recommendations(
                sort_by, limit, user_id, type, category);
            // service 返回 {data: [...], total}，拆开平铺，前端期望 data=数组
            json resp;
            if (result.contains("error")) {
                resp["code"] = 500;
                resp["message"] = result["error"];
                resp["data"] = json::array();
                resp["total"] = 0;
            } else {
                resp["code"] = 200;
                resp["message"] = "success";
                resp["data"] = result.value("data", json::array());
                resp["total"] = result.value("total", 0);
            }
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // GET /api/spots/search
    server_.Get("/api/spots/search", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string keyword = req.get_param_value("keyword");
            if (keyword.empty()) {
                res.set_content(Response::bad_request("keyword is required").dump(), "application/json");
                return;
            }
            std::string limit_str = req.get_param_value("limit");
            int limit = limit_str.empty() ? 20 : std::stoi(limit_str);
            json result = service::RecommendService::search_spots(keyword, limit);
            // search_spots 返回数组或错误对象
            json resp;
            if (result.contains("error")) {
                resp["code"] = 500;
                resp["message"] = result["error"];
                resp["data"] = json::array();
            } else {
                resp["code"] = 200;
                resp["message"] = "success";
                resp["data"] = result.is_array() ? result : json::array();
            }
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // GET /api/spots/:id
    server_.Get(R"(/api/spots/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        try {
            int spot_id = std::stoi(req.matches[1]);
            json result = service::RecommendService::get_spot_detail(spot_id);
            if (result.is_null() || !result.contains("id")) {
                res.set_content(Response::not_found("Spot not found").dump(), "application/json");
            } else {
                json resp;
                resp["code"] = 200;
                resp["message"] = "success";
                resp["data"] = result;
                res.set_content(resp.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // GET /api/spots — 分页列表
    server_.Get("/api/spots", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string sort_by = req.get_param_value("sort_by");
            if (sort_by.empty()) sort_by = "popularity";
            std::string order = req.get_param_value("order");
            if (order.empty()) order = "desc";
            std::string type = req.get_param_value("type");
            std::string category = req.get_param_value("category");
            std::string page_str = req.get_param_value("page");
            int page = page_str.empty() ? 1 : std::stoi(page_str);
            std::string size_str = req.get_param_value("page_size");
            int page_size = size_str.empty() ? 20 : std::stoi(size_str);
            if (page < 1) page = 1;
            if (page_size < 1) page_size = 20;
            if (page_size > 100) page_size = 100;

            json items = repository::SpotRepo::get_all(
                page, page_size, sort_by, order, type, category);
            int total = repository::SpotRepo::count(type, category);
            res.set_content(Response::paginated(items, total, page, page_size).dump(),
                            "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });
}

// ============================================================
// 路线规划路由
// ============================================================
void HttpServer::register_route_routes() {
    // POST /api/route/single
    server_.Post("/api/route/single", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            json result = service::RouteService::plan_single(body);
            res.set_content(Response::ok(result).dump(), "application/json");
        } catch (const json::parse_error&) {
            res.set_content(Response::bad_request("Invalid JSON").dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // POST /api/route/multi
    server_.Post("/api/route/multi", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            json result = service::RouteService::plan_multi(body);
            res.set_content(Response::ok(result).dump(), "application/json");
        } catch (const json::parse_error&) {
            res.set_content(Response::bad_request("Invalid JSON").dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // GET /api/map/graph/:areaId
    server_.Get(R"(/api/map/graph/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        try {
            int area_id = std::stoi(req.matches[1]);
            json result = service::RouteService::get_graph_data(area_id);
            res.set_content(Response::ok(result).dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    server_.Get("/api/route/indoor/buildings", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string area = req.get_param_value("area_id");
            if (area.empty()) {
                res.set_content(Response::bad_request("area_id is required").dump(), "application/json");
                return;
            }
            json result = service::IndoorRouteService::get_buildings(std::stoi(area));
            res.set_content(Response::ok(result).dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    server_.Get(R"(/api/route/indoor/graph/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json result = service::IndoorRouteService::get_graph(std::stoi(req.matches[1]));
            res.set_content(Response::ok(result).dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    server_.Post("/api/route/indoor", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json result = service::IndoorRouteService::plan(json::parse(req.body));
            res.set_content(Response::ok(result).dump(), "application/json");
        } catch (const json::parse_error&) {
            res.set_content(Response::bad_request("Invalid JSON").dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });
}

// ============================================================
// 场所查询路由
// ============================================================
void HttpServer::register_facility_routes() {
    // GET /api/facilities/nearby?area_id=1&node_id=5&category=toilet&radius=500
    server_.Get("/api/facilities/nearby", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string area_id_str = req.get_param_value("area_id");
            std::string node_id_str = req.get_param_value("node_id");
            if (area_id_str.empty() || node_id_str.empty()) {
                res.set_content(Response::bad_request("area_id and node_id are required").dump(),
                                "application/json");
                return;
            }
            int area_id = std::stoi(area_id_str);
            int node_id = std::stoi(node_id_str);
            std::string category = req.get_param_value("category");
            std::string radius_str = req.get_param_value("radius");
            double radius = radius_str.empty() ? 500.0 : std::stod(radius_str);

            json result = service::FacilityService::find_nearby(area_id, node_id, category, radius);
            res.set_content(Response::ok(result).dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });
}

// ============================================================
// 旅游日记路由
// ============================================================
void HttpServer::register_diary_routes() {
    // GET /api/diaries
    server_.Get("/api/diaries", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string sort_by = req.get_param_value("sort_by");
            if (sort_by.empty()) sort_by = "created_at";
            std::string order = req.get_param_value("order");
            if (order.empty()) order = "desc";
            std::string page_str = req.get_param_value("page");
            int page = page_str.empty() ? 1 : std::stoi(page_str);
            std::string size_str = req.get_param_value("page_size");
            int page_size = size_str.empty() ? 20 : std::stoi(size_str);
            std::string dest_str = req.get_param_value("destination_id");
            int dest_id = dest_str.empty() ? -1 : std::stoi(dest_str);

            if (page < 1) page = 1;
            if (page_size < 1) page_size = 20;
            if (page_size > 100) page_size = 100;

            json result = service::DiaryService::get_diaries(page, page_size, sort_by, order, dest_id);
            if (result.is_array()) {
                int total = repository::DiaryRepo::count(dest_id);
                res.set_content(Response::paginated(result, total, page, page_size).dump(),
                                "application/json");
            } else {
                std::string message = result.value("error", std::string("Failed to load diaries"));
                res.set_content(Response::server_error(message).dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // GET /api/diaries/:id
    server_.Get(R"(/api/diaries/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        try {
            int id = std::stoi(req.matches[1]);
            json result = service::DiaryService::get_diary(id);
            if (result.is_null() || !result.contains("id")) {
                res.set_content(Response::not_found("Diary not found").dump(), "application/json");
            } else {
                json resp;
                resp["code"] = 200;
                resp["message"] = "success";
                resp["data"] = result;
                res.set_content(resp.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // POST /api/diaries — 创建日记
    server_.Post("/api/diaries", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            json result = service::DiaryService::create_diary(body);
            json resp;
            if (result.contains("error")) {
                resp["code"] = 400;
                resp["message"] = result["error"];
                resp["data"] = nullptr;
            } else {
                resp["code"] = 200;
                resp["message"] = result.value("message", "Diary created");
                resp["data"] = result;
            }
            res.set_content(resp.dump(), "application/json");
        } catch (const json::parse_error&) {
            res.set_content(Response::bad_request("Invalid JSON").dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // PUT /api/diaries/:id — 更新日记
    server_.Put(R"(/api/diaries/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        try {
            int id = std::stoi(req.matches[1]);
            json body = json::parse(req.body);
            json result = service::DiaryService::update_diary(id, body);
            json resp;
            if (result.contains("error")) {
                resp["code"] = 400;
                resp["message"] = result["error"];
                resp["data"] = nullptr;
            } else {
                resp["code"] = 200;
                resp["message"] = result.value("message", "Diary updated");
                resp["data"] = result;
            }
            res.set_content(resp.dump(), "application/json");
        } catch (const json::parse_error&) {
            res.set_content(Response::bad_request("Invalid JSON").dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // DELETE /api/diaries/:id
    server_.Delete(R"(/api/diaries/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        try {
            int id = std::stoi(req.matches[1]);
            json result = service::DiaryService::delete_diary(id);
            json resp;
            if (result.contains("error")) {
                resp["code"] = 400;
                resp["message"] = result["error"];
                resp["data"] = nullptr;
            } else {
                resp["code"] = 200;
                resp["message"] = result.value("message", "Diary deleted");
                resp["data"] = result;
            }
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // GET /api/diaries/search?keyword=xxx&mode=fulltext&limit=20
    server_.Get("/api/diaries/search", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string keyword = req.get_param_value("keyword");
            if (keyword.empty()) {
                res.set_content(Response::bad_request("keyword is required").dump(), "application/json");
                return;
            }
            std::string mode = req.get_param_value("mode");
            if (mode.empty()) mode = "fulltext";
            std::string limit_str = req.get_param_value("limit");
            int limit = limit_str.empty() ? 20 : std::stoi(limit_str);

            json result = service::DiaryService::search_diaries(keyword, mode, limit);
            json resp;
            resp["code"] = 200;
            resp["message"] = "success";
            resp["data"] = result.value("data", json::array());
            resp["total"] = result.value("total", 0);
            resp["mode"] = result.value("mode", mode);
            if (result.contains("error")) resp["message"] = result["error"];
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // POST /api/diaries/compress — Huffman 压缩
    server_.Post("/api/diaries/compress", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            json result = service::DiaryService::compress_diary(body);
            json resp;
            if (result.contains("error")) {
                resp["code"] = 400;
                resp["message"] = result["error"];
                resp["data"] = nullptr;
            } else {
                resp["code"] = 200;
                resp["message"] = "success";
                resp["data"] = result;
            }
            res.set_content(resp.dump(), "application/json");
        } catch (const json::parse_error&) {
            res.set_content(Response::bad_request("Invalid JSON").dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // POST /api/diaries/decompress — Huffman 解压
    server_.Post("/api/diaries/decompress", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            json result = service::DiaryService::decompress_diary(body);
            json resp;
            if (result.contains("error")) {
                resp["code"] = 400;
                resp["message"] = result["error"];
                resp["data"] = nullptr;
            } else {
                resp["code"] = 200;
                resp["message"] = "success";
                resp["data"] = result;
            }
            res.set_content(resp.dump(), "application/json");
        } catch (const json::parse_error&) {
            res.set_content(Response::bad_request("Invalid JSON").dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // POST /api/diaries/:id/rate — 评分
    server_.Post(R"(/api/diaries/(\d+)/rate)", [](const httplib::Request& req, httplib::Response& res) {
        try {
            int diary_id = std::stoi(req.matches[1]);
            json body = json::parse(req.body);
            int user_id = body.value("user_id", 1);
            int score = body.value("score", 5);
            json result = service::DiaryService::rate_diary(user_id, diary_id, score);
            json resp;
            if (result.contains("error")) {
                resp["code"] = 400;
                resp["message"] = result["error"];
                resp["data"] = nullptr;
            } else {
                resp["code"] = 200;
                resp["message"] = result.value("message", "success");
                resp["data"] = result;
            }
            res.set_content(resp.dump(), "application/json");
        } catch (const json::parse_error&) {
            res.set_content(Response::bad_request("Invalid JSON").dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });
}

// ============================================================
// 美食推荐路由
// ============================================================
void HttpServer::register_food_routes() {
    // GET /api/foods/recommend?area_id=1&sort_by=rating&limit=10&cuisine=川菜
    server_.Get("/api/foods/recommend", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string area_id_str = req.get_param_value("area_id");
            if (area_id_str.empty()) {
                res.set_content(Response::bad_request("area_id is required").dump(), "application/json");
                return;
            }
            int area_id = std::stoi(area_id_str);
            std::string sort_by = req.get_param_value("sort_by");
            if (sort_by.empty()) sort_by = "rating";
            std::string cuisine = req.get_param_value("cuisine");
            std::string limit_str = req.get_param_value("limit");
            int limit = limit_str.empty() ? 10 : std::stoi(limit_str);

            json result = service::FoodService::get_recommendations(area_id, limit, sort_by, cuisine);
            json resp;
            resp["code"] = 200;
            resp["message"] = "success";
            resp["data"] = result.value("data", json::array());
            resp["total"] = result.value("total", 0);
            resp["limit"] = result.value("limit", 0);
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // GET /api/foods/search?area_id=1&keyword=宫保鸡丁&limit=20
    server_.Get("/api/foods/search", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string area_id_str = req.get_param_value("area_id");
            if (area_id_str.empty()) {
                res.set_content(Response::bad_request("area_id is required").dump(), "application/json");
                return;
            }
            int area_id = std::stoi(area_id_str);
            std::string keyword = req.get_param_value("keyword");
            std::string limit_str = req.get_param_value("limit");
            int limit = limit_str.empty() ? 20 : std::stoi(limit_str);

            json result = service::FoodService::search_foods(area_id, keyword, limit);
            json resp;
            resp["code"] = 200;
            resp["message"] = "success";
            resp["data"] = result.value("data", json::array());
            resp["total"] = result.value("total", 0);
            resp["mode"] = result.value("mode", "");
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });

    // GET /api/foods — 分页美食列表
    server_.Get("/api/foods", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string area_id_str = req.get_param_value("area_id");
            if (area_id_str.empty()) {
                res.set_content(Response::bad_request("area_id is required").dump(), "application/json");
                return;
            }
            int area_id = std::stoi(area_id_str);
            std::string sort_by = req.get_param_value("sort_by");
            if (sort_by.empty()) sort_by = "rating";
            std::string cuisine = req.get_param_value("cuisine");
            std::string page_str = req.get_param_value("page");
            int page = page_str.empty() ? 1 : std::stoi(page_str);
            std::string size_str = req.get_param_value("page_size");
            int page_size = size_str.empty() ? 20 : std::stoi(size_str);

            json foods = repository::FoodRepo::get_all(area_id, page, page_size, sort_by, cuisine);
            int total = repository::FoodRepo::count(area_id, cuisine);

            json resp;
            resp["code"] = 200;
            resp["message"] = "success";
            resp["data"] = foods;
            resp["total"] = total;
            resp["page"] = page;
            resp["page_size"] = page_size;
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(Response::server_error(e.what()).dump(), "application/json");
        }
    });
}
