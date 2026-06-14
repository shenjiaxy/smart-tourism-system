# 用户登录与管理员系统 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为现有系统增加后端会话鉴权、普通用户登录流程和独立管理员后台。

**Architecture:** 后端以现有 `users` 表为身份源，使用随机内存令牌管理会话，并通过公共鉴权辅助函数保护业务和管理员接口。前端使用 Pinia 持久化当前用户与令牌，Axios 自动携带令牌，Vue Router 根据角色切换普通布局和管理员布局。

**Tech Stack:** C++17、cpp-httplib、SQLite3、nlohmann/json、Vue 3、TypeScript、Pinia、Vue Router、Axios。

---

### Task 1: 后端身份服务

**Files:**
- Create: `smart-tourism-backend/include/service/auth_service.h`
- Modify: `smart-tourism-backend/tests/test_algorithms.cpp`

- [ ] 写登录、错误密码、会话和角色判断的失败测试。
- [ ] 实现 SQLite 预编译登录查询、随机令牌、会话查询和退出。
- [ ] 运行 `cmake-build-tests`，确认身份测试通过。

### Task 2: 后端鉴权和管理员 API

**Files:**
- Modify: `smart-tourism-backend/include/server/http_server.h`
- Modify: `smart-tourism-backend/src/server/http_server.cpp`

- [ ] 注册登录、当前用户和退出接口。
- [ ] 为业务接口增加 Bearer 令牌校验。
- [ ] 注册管理员总览、用户列表、角色更新、日记列表和日记删除接口。
- [ ] 构建后端并通过 HTTP 验证 401、403 和管理员成功响应。

### Task 3: 前端会话基础设施

**Files:**
- Create: `smart-tourism-frontend/src/api/auth.ts`
- Create: `smart-tourism-frontend/src/stores/auth.ts`
- Modify: `smart-tourism-frontend/src/utils/http.ts`
- Modify: `smart-tourism-frontend/src/types/index.ts`

- [ ] 定义用户和登录响应类型。
- [ ] 实现登录、恢复会话和退出状态。
- [ ] Axios 请求自动附加令牌，401 时清理过期会话。
- [ ] 运行 TypeScript 构建检查。

### Task 4: 登录页与路由隔离

**Files:**
- Create: `smart-tourism-frontend/src/views/LoginView.vue`
- Modify: `smart-tourism-frontend/src/router/index.ts`
- Modify: `smart-tourism-frontend/src/App.vue`
- Modify: `smart-tourism-frontend/src/components/layout/AppHeader.vue`

- [ ] 创建登录页与演示账号提示。
- [ ] 增加 `requiresAuth`、`guestOnly` 和 `adminOnly` 路由元数据。
- [ ] 实现普通用户与管理员的登录后跳转和越权重定向。
- [ ] 顶部导航显示真实用户并支持退出。

### Task 5: 管理员后台

**Files:**
- Create: `smart-tourism-frontend/src/api/admin.ts`
- Create: `smart-tourism-frontend/src/components/layout/AdminLayout.vue`
- Create: `smart-tourism-frontend/src/views/admin/AdminDashboardView.vue`
- Create: `smart-tourism-frontend/src/views/admin/AdminUsersView.vue`
- Create: `smart-tourism-frontend/src/views/admin/AdminDiariesView.vue`

- [ ] 实现管理员独立布局。
- [ ] 实现数据总览卡片。
- [ ] 实现用户列表与角色调整。
- [ ] 实现日记列表与删除。

### Task 6: 当前用户接入与最终验收

**Files:**
- Modify: `smart-tourism-frontend/src/views/DiaryView.vue`

- [ ] 创建日记和评分改用当前用户 ID。
- [ ] 运行前端测试与生产构建。
- [ ] 运行后端完整算法/集成测试。
- [ ] 重启后端但保持程序运行。
- [ ] 在浏览器分别验收普通用户与管理员登录、页面隔离和退出。

