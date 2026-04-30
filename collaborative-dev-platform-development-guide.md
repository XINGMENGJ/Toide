# 协作开发平台详细开发文档

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 构建一个以 Qt 桌面客户端为主、后端服务协调的 C/S 协作开发 IDE 平台。

**Architecture:** 客户端使用 Qt 6 + C++20 实现 IDE 体验，服务端使用 Drogon 提供 REST 与 WebSocket 能力。MVP 先完成项目工作区、文件编辑、任务运行、Git 基础操作、成员状态、文件版本和协作事件，为后续实时多人编辑预留协议和数据模型。

**Tech Stack:** Qt 6, C++20, CMake, QScintilla, libgit2, Drogon, MySQL, Redis, REST, WebSocket, JSON。

---

## 1. 项目目标

### 1.1 第一版目标

第一版要做成一个能独立使用的 Qt 桌面开发工作区，同时具备连接后端进行团队协作的基础能力。

必须完成：

- Qt 桌面主窗口。
- 项目打开和文件树。
- 基础代码编辑。
- 文件保存。
- 任务运行面板。
- Git 状态查看。
- 用户登录。
- 项目和成员管理。
- 文件版本记录。
- WebSocket 协作事件。
- 文件保存冲突检测。

不在第一版完成：

- 完整实时多人同屏编辑。
- 插件市场。
- 远程容器开发。
- 复杂 CI/CD。
- AI 自动编码。
- 浏览器版 IDE。

### 1.2 架构原则

- 采用 C/S 架构，所有团队协作状态由服务端统一协调。
- 客户端可以离线打开本地项目，但团队协作功能需要登录服务端。
- REST 用于明确的业务操作，WebSocket 用于实时事件通知。
- 文件保存接口负责持久化，WebSocket 不直接替代文件保存。
- 每个文件保存都基于版本号，服务端检测版本冲突。
- MVP 使用 JSON，后续高频实时编辑消息再考虑 Protobuf。

## 2. 总体架构

```text
Toide Qt Client
  |-- REST: login, project, member, file version
  |-- WebSocket: presence, events, notifications
  v
Toide Server
  |-- Auth Module
  |-- Team Module
  |-- Project Module
  |-- File Version Module
  |-- Collaboration Module
  |-- Git Module
  v
MySQL + Redis + File Storage
```

建议项目代号使用 `Toide`，含义可以理解为 Team-Oriented IDE。

## 3. 推荐目录结构

### 3.1 仓库结构

```text
Toide/
  CMakeLists.txt
  README.md
  docs/
    architecture.md
    api.md
    collaboration-protocol.md
    development-guide.md
  client/
    CMakeLists.txt
    src/
      main.cpp
      app/
      workspace/
      editor/
      file_explorer/
      task_runner/
      git/
      collaboration/
      network/
      local_cache/
    include/
    resources/
    tests/
  server/
    CMakeLists.txt
    src/
      main.cpp
      auth/
      team/
      project/
      file_version/
      collaboration/
      git/
      common/
    config/
    migrations/
    tests/
  scripts/
    dev-server.ps1
    run-client.ps1
```

### 3.2 客户端模块职责

`client/src/app/`

- 应用启动。
- 主窗口初始化。
- 全局配置。
- 模块装配。

`client/src/workspace/`

- 当前项目状态。
- 项目打开、关闭、刷新。
- 本地项目路径。
- 服务端项目 ID 映射。

`client/src/file_explorer/`

- 文件树视图。
- 文件打开事件。
- 文件新增、删除、重命名。

`client/src/editor/`

- 编辑器标签页。
- 文件加载和保存。
- 脏状态管理。
- 当前文件版本号。
- 基础查找和替换。

`client/src/task_runner/`

- 构建、运行、测试命令。
- 使用 `QProcess` 执行命令。
- 输出面板。
- 任务配置读取。

`client/src/git/`

- Git 状态读取。
- 分支显示。
- add、commit、pull、push 封装。
- 优先使用 `libgit2`，必要时可临时调用系统 git 命令。

`client/src/collaboration/`

- 在线成员列表。
- 当前项目协作状态。
- 文件修改事件。
- 保存冲突提示。
- 后续实时光标和选区同步。

`client/src/network/`

- REST 客户端。
- WebSocket 客户端。
- Token 管理。
- 请求错误统一处理。

`client/src/local_cache/`

- 最近打开项目。
- 登录状态缓存。
- 本地设置。
- SQLite 缓存。

### 3.3 服务端模块职责

`server/src/auth/`

- 注册。
- 登录。
- Token 生成和校验。
- 密码哈希。

`server/src/team/`

- 团队创建。
- 团队成员管理。

`server/src/project/`

- 项目创建。
- 项目列表。
- 项目成员。
- 工作区配置。

`server/src/file_version/`

- 文件版本查询。
- 文件保存。
- 内容哈希计算。
- 保存冲突检测。

`server/src/collaboration/`

- WebSocket 连接。
- 在线状态。
- 项目房间。
- 协作事件广播。

`server/src/git/`

- 仓库元数据。
- 分支信息。
- 提交记录。

`server/src/common/`

- 配置。
- 日志。
- 错误响应。
- JSON 工具。
- 数据库连接。

## 4. 客户端开发设计

### 4.1 UI 布局

第一版使用 Qt Widgets，推荐主窗口布局：

```text
+------------------------------------------------------+
| Menu / Toolbar                                       |
+-------------+---------------------------+------------+
| File Tree   | Editor Tabs               | Members    |
|             |                           | Events     |
+-------------+---------------------------+------------+
| Task Output / Git Status / Problems                  |
+------------------------------------------------------+
```

核心窗口：

- `MainWindow`
- `FileExplorerWidget`
- `EditorAreaWidget`
- `TaskRunnerWidget`
- `GitPanelWidget`
- `CollaborationPanelWidget`
- `LoginDialog`
- `ProjectOpenDialog`

### 4.2 编辑器策略

MVP 推荐使用 QScintilla：

- 先获得成熟编辑能力。
- 减少自研编辑器带来的复杂度。
- 后续可以替换为自研编辑器或封装更强的编辑器组件。

编辑器内部需要维护：

- `filePath`
- `projectId`
- `baseVersion`
- `isDirty`
- `lastSavedContentHash`

保存文件时：

1. 获取当前编辑器内容。
2. 如果是本地项目，直接写入磁盘。
3. 如果绑定了服务端项目，调用保存接口并携带 `baseVersion`。
4. 保存成功后更新 `baseVersion`。
5. 保存冲突时弹出冲突处理窗口。

### 4.3 任务运行

任务配置建议使用项目根目录下的 `.toide/tasks.json`：

```json
{
  "tasks": [
    {
      "name": "Configure",
      "command": "cmake -S . -B build",
      "workingDirectory": "${workspaceRoot}"
    },
    {
      "name": "Build",
      "command": "cmake --build build",
      "workingDirectory": "${workspaceRoot}"
    },
    {
      "name": "Test",
      "command": "ctest --test-dir build --output-on-failure",
      "workingDirectory": "${workspaceRoot}"
    }
  ]
}
```

客户端使用 `QProcess` 执行任务，将 stdout 和 stderr 输出到任务面板。

### 4.4 本地缓存

SQLite 表建议：

```sql
CREATE TABLE local_settings (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

CREATE TABLE recent_projects (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL,
  path TEXT NOT NULL,
  server_project_id TEXT,
  last_opened_at TEXT NOT NULL
);

CREATE TABLE cached_file_versions (
  project_id TEXT NOT NULL,
  file_path TEXT NOT NULL,
  version INTEGER NOT NULL,
  content_hash TEXT NOT NULL,
  updated_at TEXT NOT NULL,
  PRIMARY KEY (project_id, file_path)
);
```

## 5. 服务端开发设计

### 5.1 服务端框架

推荐 Drogon：

- HTTP 路由清晰。
- 支持 WebSocket。
- 支持数据库访问。
- 性能足够好。
- C++ 项目一致性较强。

### 5.2 数据库表

MySQL 初始表：

```sql
CREATE TABLE users (
  id CHAR(36) PRIMARY KEY,
  username VARCHAR(64) NOT NULL UNIQUE,
  email VARCHAR(255) NOT NULL UNIQUE,
  password_hash TEXT NOT NULL,
  created_at DATETIME(3) NOT NULL,
  last_login_at DATETIME(3)
);

CREATE TABLE teams (
  id CHAR(36) PRIMARY KEY,
  name VARCHAR(128) NOT NULL,
  owner_id CHAR(36) NOT NULL,
  created_at DATETIME(3) NOT NULL,
  CONSTRAINT fk_teams_owner FOREIGN KEY (owner_id) REFERENCES users(id)
);

CREATE TABLE projects (
  id CHAR(36) PRIMARY KEY,
  team_id CHAR(36) NOT NULL,
  name VARCHAR(128) NOT NULL,
  description TEXT NOT NULL DEFAULT '',
  repository_url TEXT NOT NULL DEFAULT '',
  workspace_config JSON NOT NULL,
  created_at DATETIME(3) NOT NULL,
  CONSTRAINT fk_projects_team FOREIGN KEY (team_id) REFERENCES teams(id)
);

CREATE TABLE project_members (
  id CHAR(36) PRIMARY KEY,
  project_id CHAR(36) NOT NULL,
  user_id CHAR(36) NOT NULL,
  role VARCHAR(32) NOT NULL,
  joined_at DATETIME(3) NOT NULL,
  UNIQUE(project_id, user_id),
  CONSTRAINT fk_project_members_project FOREIGN KEY (project_id) REFERENCES projects(id),
  CONSTRAINT fk_project_members_user FOREIGN KEY (user_id) REFERENCES users(id)
);

CREATE TABLE file_versions (
  id CHAR(36) PRIMARY KEY,
  project_id CHAR(36) NOT NULL,
  file_path TEXT NOT NULL,
  version BIGINT NOT NULL,
  content_hash VARCHAR(128) NOT NULL,
  updated_by CHAR(36) NOT NULL,
  updated_at DATETIME(3) NOT NULL,
  UNIQUE(project_id, file_path, version),
  CONSTRAINT fk_file_versions_project FOREIGN KEY (project_id) REFERENCES projects(id),
  CONSTRAINT fk_file_versions_user FOREIGN KEY (updated_by) REFERENCES users(id)
);

CREATE TABLE collaboration_events (
  id CHAR(36) PRIMARY KEY,
  project_id CHAR(36) NOT NULL,
  user_id CHAR(36) NOT NULL,
  event_type VARCHAR(64) NOT NULL,
  payload JSON NOT NULL,
  created_at DATETIME(3) NOT NULL,
  CONSTRAINT fk_collaboration_events_project FOREIGN KEY (project_id) REFERENCES projects(id),
  CONSTRAINT fk_collaboration_events_user FOREIGN KEY (user_id) REFERENCES users(id)
);
```

关键索引：

```sql
CREATE INDEX idx_projects_team_id ON projects(team_id);
CREATE INDEX idx_project_members_project_id ON project_members(project_id);
CREATE INDEX idx_file_versions_project_path ON file_versions(project_id, file_path);
CREATE INDEX idx_collaboration_events_project_created ON collaboration_events(project_id, created_at DESC);
```

在线状态不建议长期保存在 MySQL，优先放 Redis：

```text
presence:project:{projectId} -> set(userId)
session:{clientId} -> userId, projectId, currentFile, lastSeenAt
```

## 6. API 设计

### 6.1 认证接口

`POST /api/auth/register`

请求：

```json
{
  "username": "alice",
  "email": "alice@example.com",
  "password": "passw0rd"
}
```

响应：

```json
{
  "userId": "uuid",
  "username": "alice"
}
```

`POST /api/auth/login`

请求：

```json
{
  "email": "alice@example.com",
  "password": "passw0rd"
}
```

响应：

```json
{
  "accessToken": "jwt-token",
  "user": {
    "id": "uuid",
    "username": "alice",
    "email": "alice@example.com"
  }
}
```

### 6.2 项目接口

`GET /api/projects`

返回当前用户可访问的项目列表。

`POST /api/projects`

创建项目。

`GET /api/projects/{projectId}`

获取项目详情。

`GET /api/projects/{projectId}/members`

获取项目成员。

### 6.3 文件版本接口

`GET /api/projects/{projectId}/files/version?path=src/main.cpp`

响应：

```json
{
  "projectId": "uuid",
  "filePath": "src/main.cpp",
  "version": 12,
  "contentHash": "sha256-value",
  "updatedAt": "2026-04-29T17:21:00+08:00"
}
```

`PUT /api/projects/{projectId}/files/content`

请求：

```json
{
  "filePath": "src/main.cpp",
  "baseVersion": 12,
  "content": "#include <iostream>\nint main() { return 0; }\n"
}
```

保存成功：

```json
{
  "status": "saved",
  "filePath": "src/main.cpp",
  "version": 13,
  "contentHash": "sha256-value"
}
```

版本冲突：

```json
{
  "error": {
    "code": "FILE_VERSION_CONFLICT",
    "message": "The file has been updated by another user.",
    "latestVersion": 14,
    "baseVersion": 12
  }
}
```

## 7. WebSocket 协作协议

连接地址：

```text
wss://server.example.com/ws/projects/{projectId}?token={accessToken}
```

### 7.1 客户端发送事件

加入项目：

```json
{
  "type": "presence.join",
  "clientId": "client-001",
  "projectId": "project-001"
}
```

切换当前文件：

```json
{
  "type": "presence.current_file",
  "clientId": "client-001",
  "projectId": "project-001",
  "filePath": "src/main.cpp"
}
```

心跳：

```json
{
  "type": "heartbeat",
  "clientId": "client-001",
  "timestamp": "2026-04-29T17:21:00+08:00"
}
```

### 7.2 服务端广播事件

成员上线：

```json
{
  "type": "presence.user_joined",
  "projectId": "project-001",
  "user": {
    "id": "user-001",
    "username": "alice"
  }
}
```

文件更新：

```json
{
  "type": "file.updated",
  "projectId": "project-001",
  "filePath": "src/main.cpp",
  "version": 13,
  "updatedBy": {
    "id": "user-001",
    "username": "alice"
  },
  "timestamp": "2026-04-29T17:21:00+08:00"
}
```

当前文件变化：

```json
{
  "type": "presence.current_file_changed",
  "projectId": "project-001",
  "userId": "user-001",
  "filePath": "src/main.cpp"
}
```

### 7.3 后续实时编辑事件

第三阶段再启用：

```json
{
  "type": "editor.operation",
  "projectId": "project-001",
  "filePath": "src/main.cpp",
  "baseVersion": 13,
  "operation": {
    "kind": "insert",
    "position": 42,
    "text": "return 0;"
  }
}
```

## 8. 错误处理规范

统一错误响应：

```json
{
  "error": {
    "code": "PROJECT_NOT_FOUND",
    "message": "Project does not exist or you do not have permission.",
    "requestId": "req-001"
  }
}
```

常见错误码：

- `UNAUTHORIZED`
- `FORBIDDEN`
- `PROJECT_NOT_FOUND`
- `FILE_NOT_FOUND`
- `FILE_VERSION_CONFLICT`
- `INVALID_REQUEST`
- `WEBSOCKET_AUTH_FAILED`
- `INTERNAL_ERROR`

客户端处理原则：

- 认证失败：跳转登录。
- 权限不足：显示明确提示。
- 文件冲突：显示冲突对话框。
- 网络断开：进入离线状态，协作面板提示重连中。
- WebSocket 断开：指数退避重连。

## 9. 开发里程碑

### Milestone 1：Qt 客户端骨架

- [ ] 创建 CMake 工程。
- [ ] 创建 Qt Widgets 主窗口。
- [ ] 创建菜单栏和工具栏。
- [ ] 创建左侧文件树区域。
- [ ] 创建中间编辑器区域。
- [ ] 创建底部输出面板。
- [ ] 创建右侧协作面板容器。
- [ ] 添加基本单元测试工程。

验收标准：

- 可以启动空白 IDE 主界面。
- 主窗口布局稳定。
- Debug 和 Release 都能构建。

### Milestone 2：本地项目工作区

- [ ] 打开本地目录。
- [ ] 显示文件树。
- [ ] 双击文件打开编辑器标签页。
- [ ] 保存文件。
- [ ] 显示文件脏状态。
- [ ] 支持最近项目。

验收标准：

- 可以打开一个 C++ 项目目录。
- 可以编辑并保存文本文件。
- 重启后能看到最近项目。

### Milestone 3：任务运行

- [ ] 读取 `.toide/tasks.json`。
- [ ] 显示任务列表。
- [ ] 使用 `QProcess` 执行任务。
- [ ] 输出 stdout 和 stderr。
- [ ] 支持终止任务。

验收标准：

- 可以运行 CMake configure/build/test 命令。
- 输出面板实时显示命令输出。

### Milestone 4：Git 基础能力

- [ ] 检测项目是否为 Git 仓库。
- [ ] 显示 changed/untracked/staged 文件。
- [ ] 支持 stage 文件。
- [ ] 支持提交。
- [ ] 显示当前分支。

验收标准：

- 可以查看 Git 状态。
- 可以完成一次本地提交。

### Milestone 5：后端基础服务

- [ ] 创建 Drogon 工程。
- [ ] 配置 MySQL。
- [ ] 创建数据库迁移。
- [ ] 实现注册和登录。
- [ ] 实现 Token 认证中间件。
- [ ] 实现项目 CRUD。
- [ ] 实现项目成员接口。

验收标准：

- 可以通过 HTTP 客户端注册、登录、创建项目。
- 非法 Token 无法访问受保护接口。

### Milestone 6：客户端接入后端

- [ ] 实现 REST 客户端封装。
- [ ] 实现登录窗口。
- [ ] 保存 access token。
- [ ] 拉取项目列表。
- [ ] 绑定本地目录和服务端项目。

验收标准：

- Qt 客户端可以登录。
- 可以看到服务端项目列表。

### Milestone 7：文件版本和冲突检测

- [ ] 服务端实现文件版本查询。
- [ ] 服务端实现保存文件内容。
- [ ] 保存时校验 `baseVersion`。
- [ ] 客户端保存时携带版本号。
- [ ] 客户端处理冲突响应。

验收标准：

- 两个客户端基于同一版本修改同一文件时，后保存的一方收到冲突提示。

### Milestone 8：WebSocket 协作事件

- [ ] 服务端实现 WebSocket 鉴权。
- [ ] 服务端实现项目房间。
- [ ] 客户端连接项目 WebSocket。
- [ ] 实现在线成员列表。
- [ ] 保存文件后广播 `file.updated`。
- [ ] 客户端协作面板显示事件。

验收标准：

- 两个客户端打开同一项目时能看到在线成员。
- 一个客户端保存文件后，另一个客户端能收到文件更新事件。

## 10. 测试策略

### 10.1 客户端测试

- 使用 Qt Test 测试纯逻辑类。
- UI 行为优先做手动验收，后续再补自动化 UI 测试。
- `NetworkClient` 使用 mock HTTP/WebSocket 服务测试。
- `TaskRunner` 使用简单命令测试输出捕获。

重点测试：

- 文件打开和保存。
- 最近项目缓存。
- 任务配置解析。
- WebSocket 断线重连。
- 文件版本冲突处理。

### 10.2 服务端测试

- 使用 Drogon 自带测试能力或 Catch2。
- API 使用集成测试。
- 数据库测试使用测试库或容器。

重点测试：

- 注册登录。
- Token 鉴权。
- 项目权限。
- 文件版本递增。
- 版本冲突。
- WebSocket 鉴权。

### 10.3 手动联调场景

场景一：单客户端本地开发

1. 打开本地项目。
2. 修改文件。
3. 保存文件。
4. 运行构建任务。
5. 查看 Git 状态。

场景二：双客户端协作

1. 两个客户端登录不同账号。
2. 加入同一个项目。
3. 同时打开同一文件。
4. 客户端 A 保存。
5. 客户端 B 收到文件更新事件。
6. 客户端 B 基于旧版本保存时收到冲突提示。

## 11. 开发规范

### 11.1 C++ 规范

- 使用 C++20。
- 优先 RAII，避免裸 `new` 和手动 `delete`。
- Qt 对象父子关系明确。
- UI 类只处理界面，不直接写复杂业务逻辑。
- 网络请求统一经过 `NetworkClient`。
- 错误使用明确枚举或错误对象，不用字符串到处判断。

### 11.2 命名建议

- 类名：`PascalCase`，如 `WorkspaceManager`。
- 函数名：Qt 风格可使用 `camelCase`，如 `openProject()`。
- 文件名：建议 `snake_case`，如 `workspace_manager.cpp`。
- JSON 字段：`camelCase`，如 `projectId`。
- 数据库字段：`snake_case`，如 `project_id`。

### 11.3 提交建议

如果后续初始化 git，提交粒度建议：

- `chore: initialize qt client project`
- `feat: add local workspace file explorer`
- `feat: add editor tabs and file save`
- `feat: add task runner panel`
- `feat: add server auth endpoints`
- `feat: add file version conflict detection`
- `feat: add websocket collaboration events`

## 12. 后续实时编辑预留

从第一版开始保留这些概念：

- `CollaborationSession`
- `clientId`
- `projectId`
- `filePath`
- `baseVersion`
- `operationId`
- `serverRevision`

实时编辑实现前，需要单独调研：

- OT 文本操作模型。
- CRDT 文本结构。
- 光标位置在文本变更后的转换。
- 离线编辑和重连补偿。
- 大文件性能限制。

建议第三阶段先做单文件协作编辑原型，不要直接覆盖所有文件类型。

## 13. 当前最小启动任务

最推荐先做这 5 件事：

1. 初始化 Qt 6 + CMake 客户端工程。
2. 做出主窗口布局。
3. 做出本地项目文件树。
4. 做出基础编辑器标签页。
5. 做出任务运行面板。

这 5 件完成后，项目就有可见成果，再开始接后端和协作能力。
