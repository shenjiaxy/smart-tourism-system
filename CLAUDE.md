# 智能旅游系统 - Claude Code 配置

## 核心规则

### 思考强度
- 所有代码变更前必须充分分析影响范围
- 修改多个文件时，先理清依赖关系再动手
- 不写无意义的注释，代码即文档

### 自动审查
- 每次修改完代码后，自动执行以下检查：
  - 前端：`npx vue-tsc --noEmit` 确保类型正确
  - 后端：直接 g++ 编译检查语法错误
  - 检查是否引入未使用的 import/变量
  - 检查是否覆盖了所有错误分支
- 发现任何问题立即修复，不等到最后

### 代码风格
- 默认不加注释，只在 WHY 不明显的场景加一行短注释
- 不引入过度抽象，3行重复好过1个不成熟的抽象
- 不添加没必要的错误处理 — 只在校验系统边界（用户输入、外部API）时加
- 编辑已有文件，不随意新建文件

### 技术栈
- 前端：Vue 3 + TypeScript + Element Plus，构建目录 `smart-tourism-frontend/`
- 后端：C++17 + cpp-httplib + nlohmann/json + SQLite3，源码 `smart-tourism-backend/`
- 构建：前端 `npm run build`，后端直接 g++ 编译
- 数据生成：`node data/generate_demo_data.mjs` 重新生成种子数据
