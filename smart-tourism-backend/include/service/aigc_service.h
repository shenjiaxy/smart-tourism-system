/**
 * @file aigc_service.h
 * @brief AIGC 图生视频服务 —— 代理用户配置的 OpenAI 兼容 API
 *
 * 流程:
 *   1. 前端上传图片(base64) + prompt + API 配置
 *   2. 后端校验参数
 *   3. 代理请求到用户配置的第三方 API
 *   4. 返回生成结果给前端
 */

#pragma once

#include "json.hpp"
#include "httplib.h"
#include <string>
#include <regex>

using json = nlohmann::json;

namespace service {

class AigcService {
public:
    static json generate_video(const json& body) {
        // ========== 1. 参数校验 ==========
        std::string image = body.value("image", "");
        if (image.empty()) {
            return {{"error", "图片不能为空"}};
        }

        std::string prompt = body.value("prompt", "");
        if (prompt.empty()) {
            return {{"error", "描述提示词不能为空"}};
        }

        std::string api_base_url = body.value("api_base_url", "");
        if (api_base_url.empty()) {
            return {{"error", "API 地址不能为空"}};
        }

        std::string api_key = body.value("api_key", "");
        if (api_key.empty()) {
            return {{"error", "API Key 不能为空"}};
        }

        std::string model = body.value("model", "cogvideo-3");
        std::string endpoint = body.value("endpoint", "/v1/video/generations");

        // ========== 2. 去除 URL 末尾斜杠 ==========
        while (!api_base_url.empty() && api_base_url.back() == '/') {
            api_base_url.pop_back();
        }

        // ========== 3. 构造完整 URL ==========
        std::string full_url = api_base_url + endpoint;

        // ========== 4. 构造请求体 ==========
        json req_body;
        req_body["model"] = model;
        req_body["prompt"] = prompt;
        req_body["image"] = image;

        // ========== 5. 发送 HTTP 请求 ==========
        try {
            httplib::Client cli(api_base_url);
            cli.set_connection_timeout(30);
            cli.set_read_timeout(120, 0);

            httplib::Headers headers = {
                {"Authorization", "Bearer " + api_key},
                {"Content-Type", "application/json"}
            };

            auto res = cli.Post(endpoint, headers, req_body.dump(), "application/json");

            if (!res) {
                auto err = cli.get_openssl_verify_result();
                if (err != 0) {
                    return {{"error", "SSL 证书验证失败"}};
                }
                return {{"error", "连接 API 服务器失败，请检查 API 地址"}};
            }

            if (res->status < 200 || res->status >= 300) {
                json err_resp;
                err_resp["error"] = "外部 API 返回错误 (HTTP " + std::to_string(res->status) + ")";
                err_resp["detail"] = res->body;
                err_resp["http_status"] = res->status;
                return err_resp;
            }

            // ========== 6. 解析并返回结果 ==========
            try {
                json api_result = json::parse(res->body);

                // 尝试提取视频 URL (常见字段名)
                std::string video_url;
                if (api_result.contains("data")) {
                    auto& data = api_result["data"];
                    if (data.is_array() && !data.empty()) {
                        video_url = data[0].value("url", data[0].value("video_url", ""));
                    } else if (data.is_object()) {
                        video_url = data.value("url", data.value("video_url", ""));
                    }
                }
                if (video_url.empty()) {
                    video_url = api_result.value("video_url",
                        api_result.value("url", ""));
                }

                json result;
                result["success"] = true;
                result["video_url"] = video_url;
                result["raw_response"] = api_result;
                return result;

            } catch (const json::parse_error&) {
                // 如果响应不是 JSON，检查是否是直接的视频 URL
                std::string resp_body = res->body;
                if (resp_body.find("http") == 0 && resp_body.size() < 2048) {
                    json result;
                    result["success"] = true;
                    result["video_url"] = resp_body;
                    return result;
                }

                json result;
                result["success"] = true;
                result["raw_body"] = resp_body;
                return result;
            }

        } catch (const std::exception& e) {
            return {{"error", std::string("请求失败: ") + e.what()}};
        }
    }
};

} // namespace service
