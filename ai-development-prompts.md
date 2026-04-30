# Toide AI 开发提示词

这份文件用于后续把 Toide 协作开发平台拆给 AI 分阶段开发。建议每次只让 AI 完成一个小任务，完成后先运行构建和测试，再继续下一步。

## 1. 通用项目上下文提示词

每次新开 AI 对话时，可以先发送这一段：

```text
你是一个资深 C++/Qt 和后端开发工程师。我要开发一个名为 Toide 的协作开发平台。

项目定位：
- Toide 是一个 C/S 架构的协作开发 IDE。
- Client 是 Qt 6 + C++20 桌面客户端。
- Server 是后端服务，优先考虑 Drogon + MySQL + Redis。
- MVP 目标是项目工作区、文件树、代码编辑、任务运行、Git 基础操作、成员状态、文件版本和 WebSocket 协作事件。
- 第一版不实现完整多人实时编辑，但必须从架构上预留 projectId、filePath、baseVersion、clientId、CollaborationSession、WebSocket event 等概念。

开发原则：
- 优先使用 Qt Widgets。
- 使用 CMake 管理工程。
- 客户端模块要清晰拆分：app、workspace、file_explorer、editor、task_runner、git、collaboration、network、local_cache。
- 服务端模块要清晰拆分：auth、team、project、file_version、collaboration、git、common。
- REST 用于业务操作，WebSocket 用于协作事件。
- 文件保存必须携带 baseVersion，服务端负责冲突检测。
- 不要一次性实现过多功能，每次只完成一个可构建、可验证的小任务。

请先阅读已有文档：
- collaborative-dev-platform-brainstorm.md
- collaborative-dev-platform-development-guide.md

之后根据我给你的具体任务进行开发。修改代码前先说明计划，修改后说明改了哪些文件，并给出构建或测试命令。
```

## 2. 初始化 Qt 客户端工程提示词

```text
请为 Toide 初始化 Qt 6 + C++20 + CMake 客户端工程。

目标：
- 在 client/ 下创建 Qt Widgets 应用。
- 创建 MainWindow。
- 主窗口包含菜单栏、工具栏、左侧文件树区域、中间编辑器区域、右侧协作面板、底部输出面板。
- 暂时只做 UI 骨架，不实现真实业务。

技术要求：
- 使用 CMake。
- 使用 Qt 6 Widgets。
- C++ 标准为 C++20。
- 文件拆分清晰，不要把所有逻辑写进 main.cpp。

建议文件：
- client/CMakeLists.txt
- client/src/main.cpp
- client/src/app/main_window.h
- client/src/app/main_window.cpp
- client/src/app/main_window.ui 或纯 C++ 构建 UI

验收标准：
- 可以成功配置和构建。
- 启动后能看到 Toide 主窗口。
- 主窗口标题为 Toide。
- 界面区域布局清楚，后续可以继续扩展。

请完成实现，并给出 Windows PowerShell 下的构建命令。
```

## 3. 本地项目文件树提示词

```text
请为 Toide Qt 客户端实现本地项目打开和文件树功能。

目标：
- 用户可以通过菜单选择一个本地目录作为项目。
- 左侧文件树显示该目录内容。
- 文件树需要区分文件和目录。
- 双击文件时发出打开文件的信号，但暂时可以只在状态栏显示文件路径。
- 记录最近打开的项目路径，可以先使用 QSettings，后续再迁移到 SQLite。

技术要求：
- 使用 Qt Widgets。
- 文件树可以使用 QFileSystemModel + QTreeView。
- 逻辑放在 file_explorer 模块，不要堆在 MainWindow。

建议文件：
- client/src/file_explorer/file_explorer_widget.h
- client/src/file_explorer/file_explorer_widget.cpp
- client/src/workspace/workspace_manager.h
- client/src/workspace/workspace_manager.cpp

验收标准：
- 菜单中有“打开项目”。
- 选择目录后，左侧显示目录树。
- 双击文件时，状态栏显示文件绝对路径。
- 重新打开程序后可以看到最近项目入口或记录。

请先给出实现计划，再修改代码。
```

## 4. 编辑器标签页提示词

```text
请为 Toide Qt 客户端实现基础编辑器标签页。

目标：
- 双击文件树中的文本文件后，在中间区域打开一个编辑器标签页。
- 同一个文件重复打开时切换到已有标签页。
- 编辑后标签页显示脏状态。
- 支持保存当前文件。
- 支持关闭标签页。

技术选择：
- MVP 可以先使用 QPlainTextEdit。
- 后续再替换为 QScintilla。

建议文件：
- client/src/editor/editor_area_widget.h
- client/src/editor/editor_area_widget.cpp
- client/src/editor/editor_tab.h
- client/src/editor/editor_tab.cpp

验收标准：
- 可以打开 .cpp、.h、.txt、.md 文件。
- 可以编辑并保存。
- 保存后文件内容真实写入磁盘。
- 未保存修改关闭时要提示用户。

请注意：
- 不要在 MainWindow 中直接处理文件读写细节。
- EditorTab 维护 filePath 和 isDirty。
```

## 5. 任务运行面板提示词

```text
请为 Toide Qt 客户端实现任务运行面板。

目标：
- 读取项目根目录下的 .toide/tasks.json。
- 在底部面板显示任务列表。
- 用户可以运行任务。
- 使用 QProcess 执行命令。
- 实时显示 stdout 和 stderr。
- 支持停止正在运行的任务。

tasks.json 示例：
{
  "tasks": [
    {
      "name": "Build",
      "command": "cmake --build build",
      "workingDirectory": "${workspaceRoot}"
    }
  ]
}

建议文件：
- client/src/task_runner/task_runner_widget.h
- client/src/task_runner/task_runner_widget.cpp
- client/src/task_runner/task_config.h
- client/src/task_runner/task_config.cpp

验收标准：
- 打开项目后能读取 .toide/tasks.json。
- 能运行任务并实时显示输出。
- 工作目录正确替换 ${workspaceRoot}。
- 可以停止任务。

请包含必要的错误提示，例如 tasks.json 不存在、JSON 格式错误、任务命令为空。
```

## 6. Git 面板提示词

```text
请为 Toide Qt 客户端实现 Git 状态面板。

目标：
- 检测当前项目是否是 Git 仓库。
- 显示当前分支。
- 显示 changed、staged、untracked 文件。
- 支持刷新状态。
- 支持 stage 单个文件。
- 支持输入 commit message 并提交。

技术要求：
- 优先使用 libgit2。
- 如果当前环境暂时没有 libgit2，可以先封装 GitService 接口，内部临时用 QProcess 调用 git 命令，但接口要方便后续替换成 libgit2。

建议文件：
- client/src/git/git_panel_widget.h
- client/src/git/git_panel_widget.cpp
- client/src/git/git_service.h
- client/src/git/git_service.cpp
- client/src/git/git_status.h

验收标准：
- 可以显示 Git 仓库状态。
- 可以 stage 文件。
- 可以完成一次 commit。
- 所有 git 错误都显示在 UI 上，而不是程序崩溃。
```

## 7. Drogon 后端初始化提示词

```text
请为 Toide 创建 Drogon C++ 后端工程。

目标：
- 在 server/ 下创建 Drogon 服务。
- 提供健康检查接口 GET /api/health。
- 配置 MySQL 和 Redis 连接参数。
- 预留 auth、team、project、file_version、collaboration、common 模块。

技术要求：
- 使用 CMake。
- C++20。
- 配置文件放在 server/config/。
- 数据库迁移 SQL 放在 server/migrations/。
- 日志要清晰。

建议文件：
- server/CMakeLists.txt
- server/src/main.cpp
- server/src/common/config.h
- server/src/common/config.cpp
- server/src/common/api_response.h
- server/src/common/api_response.cpp
- server/src/health/health_controller.h
- server/src/health/health_controller.cpp
- server/config/development.json

验收标准：
- 后端可以启动。
- GET /api/health 返回 {"status":"ok"}。
- 配置文件读取失败时有明确错误。

请给出 Windows 和 Linux 下的构建运行命令。
```

## 8. 认证接口提示词

```text
请为 Toide 后端实现用户注册和登录接口。

接口：
- POST /api/auth/register
- POST /api/auth/login

数据库表：
- users(id, username, email, password_hash, created_at, last_login_at)

要求：
- 密码不能明文保存。
- 登录成功返回 accessToken 和用户信息。
- 受保护接口后续通过 Authorization: Bearer <token> 鉴权。
- 输入校验要明确，例如邮箱为空、密码过短、用户名重复、邮箱重复。

建议文件：
- server/src/auth/auth_controller.h
- server/src/auth/auth_controller.cpp
- server/src/auth/auth_service.h
- server/src/auth/auth_service.cpp
- server/src/auth/password_hasher.h
- server/src/auth/password_hasher.cpp
- server/src/auth/token_service.h
- server/src/auth/token_service.cpp
- server/migrations/001_create_users.sql

验收标准：
- 可以注册新用户。
- 重复邮箱注册失败。
- 正确密码可以登录。
- 错误密码登录失败。
- 返回的错误响应符合统一格式。

请同时补充接口测试或给出 curl 测试命令。
```

## 9. 项目和成员接口提示词

```text
请为 Toide 后端实现团队、项目和成员接口。

目标：
- 创建团队。
- 创建项目。
- 获取当前用户可访问的项目列表。
- 获取项目详情。
- 获取项目成员列表。
- 添加项目成员。

数据库表：
- teams
- projects
- project_members

权限规则：
- owner 可以管理项目成员。
- maintainer 可以修改项目设置。
- developer 可以访问项目和提交文件版本。
- viewer 只能查看。

接口建议：
- POST /api/teams
- GET /api/projects
- POST /api/projects
- GET /api/projects/{projectId}
- GET /api/projects/{projectId}/members
- POST /api/projects/{projectId}/members

验收标准：
- 登录用户可以创建团队和项目。
- 项目创建者自动成为 owner。
- 非项目成员无法访问项目详情。
- viewer 无法添加成员。
```

## 10. 文件版本和冲突检测提示词

```text
请为 Toide 后端实现文件版本记录和保存冲突检测。

目标：
- 查询文件当前版本。
- 保存文件内容。
- 保存时检查 baseVersion。
- 如果 baseVersion 不是最新版本，返回 FILE_VERSION_CONFLICT。
- 保存成功后创建新版本，并记录 contentHash。
- 保存成功后发布 file.updated 协作事件。

接口：
- GET /api/projects/{projectId}/files/version?path=src/main.cpp
- PUT /api/projects/{projectId}/files/content

请求示例：
{
  "filePath": "src/main.cpp",
  "baseVersion": 12,
  "content": "..."
}

成功响应：
{
  "status": "saved",
  "filePath": "src/main.cpp",
  "version": 13,
  "contentHash": "..."
}

冲突响应：
{
  "error": {
    "code": "FILE_VERSION_CONFLICT",
    "message": "The file has been updated by another user.",
    "latestVersion": 14,
    "baseVersion": 12
  }
}

验收标准：
- 第一次保存文件创建 version 1。
- 基于最新版本保存会递增版本。
- 基于旧版本保存会返回冲突。
- 没有项目权限的用户不能保存。
```

## 11. WebSocket 协作事件提示词

```text
请为 Toide 实现 WebSocket 协作事件通道。

目标：
- 客户端连接 /ws/projects/{projectId}?token=...
- 服务端校验 token 和项目权限。
- 服务端维护项目房间。
- 用户加入项目后广播 presence.user_joined。
- 用户断开后广播 presence.user_left。
- 客户端发送 presence.current_file 时广播 current_file_changed。
- 文件保存成功后广播 file.updated。

事件格式：
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

验收标准：
- 两个客户端连接同一项目，可以互相看到上线下线事件。
- 一个客户端切换文件，另一个客户端收到当前文件事件。
- 一个客户端保存文件，另一个客户端收到 file.updated。
- 非项目成员无法建立 WebSocket 连接。
```

## 12. Qt 客户端接入登录和项目列表提示词

```text
请为 Toide Qt 客户端接入后端登录和项目列表。

目标：
- 创建 LoginDialog。
- 用户输入邮箱和密码后调用 POST /api/auth/login。
- 登录成功后保存 accessToken。
- 拉取 GET /api/projects。
- 显示项目列表。
- 用户可以选择一个服务端项目并绑定当前本地目录。

建议文件：
- client/src/network/api_client.h
- client/src/network/api_client.cpp
- client/src/network/auth_session.h
- client/src/network/auth_session.cpp
- client/src/app/login_dialog.h
- client/src/app/login_dialog.cpp
- client/src/workspace/project_selector_dialog.h
- client/src/workspace/project_selector_dialog.cpp

验收标准：
- 登录失败时显示错误。
- 登录成功后主窗口显示用户名。
- 可以看到项目列表。
- accessToken 会用于后续请求。
```

## 13. Qt 客户端接入文件版本提示词

```text
请为 Toide Qt 客户端接入服务端文件版本和保存冲突检测。

目标：
- 打开绑定服务端项目的文件时，查询文件版本。
- EditorTab 保存时携带 baseVersion。
- 保存成功后更新 baseVersion。
- 保存冲突时弹出冲突提示窗口。
- 冲突提示至少包含：本地版本、服务端最新版本、文件路径。

要求：
- 本地项目未绑定服务端时，仍然可以正常本地保存。
- 网络失败时不要丢失用户编辑内容。
- 保存失败后标签页保持脏状态。

验收标准：
- 正常保存可以更新版本号。
- 旧版本保存会显示冲突。
- 网络断开时显示错误并保留编辑内容。
```

## 14. Qt 客户端接入 WebSocket 提示词

```text
请为 Toide Qt 客户端接入 WebSocket 协作事件。

目标：
- 打开服务端项目后连接 WebSocket。
- 协作面板显示在线成员。
- 协作面板显示文件更新事件。
- 切换编辑器标签页时发送 presence.current_file。
- WebSocket 断开后自动重连。

建议文件：
- client/src/collaboration/collaboration_client.h
- client/src/collaboration/collaboration_client.cpp
- client/src/collaboration/collaboration_panel_widget.h
- client/src/collaboration/collaboration_panel_widget.cpp
- client/src/collaboration/collaboration_event.h

验收标准：
- 两个客户端打开同一项目时能看到在线成员变化。
- 一个客户端保存文件后，另一个客户端协作面板显示更新事件。
- 断网后 UI 显示重连中，恢复后能重新连接。
```

## 15. 代码审查提示词

```text
请以资深 C++/Qt 工程师的身份审查当前 Toide 代码。

重点关注：
- Qt 对象生命周期是否安全。
- 是否存在 UI 线程阻塞。
- QProcess、QNetworkAccessManager、QWebSocket 的错误处理是否完整。
- 模块边界是否清晰。
- MainWindow 是否承担了过多业务逻辑。
- 文件保存是否可能丢失用户内容。
- WebSocket 重连是否可能造成重复连接。
- 后端权限检查是否遗漏。
- 文件版本冲突检测是否可靠。

输出要求：
- 先列出严重问题。
- 每个问题说明影响、位置和建议修复方式。
- 如果没有严重问题，说明剩余风险。
- 不要只做风格建议，要优先找真实 bug 和架构风险。
```

## 16. 测试生成提示词

```text
请为当前 Toide 模块补充测试。

测试原则：
- 优先测试纯逻辑类。
- UI 测试只覆盖关键交互。
- 网络模块使用 mock 或本地测试服务。
- 服务端 API 使用集成测试。

请根据当前代码自动判断需要添加哪些测试，并优先覆盖：
- 文件打开和保存。
- 任务配置解析。
- Git 状态解析。
- 登录失败和成功。
- 项目权限。
- 文件版本递增。
- 文件版本冲突。
- WebSocket 事件解析。

输出要求：
- 先说明测试计划。
- 再添加测试代码。
- 最后给出运行命令和预期结果。
```

## 17. Bug 修复提示词

```text
请帮我修复 Toide 中的这个问题：

问题描述：
在这里粘贴现象。

复现步骤：
1. 写清第一步操作。
2. 写清第二步操作。
3. 写清第三步操作。

期望结果：
说明应该发生什么。

实际结果：
说明实际发生什么。

相关日志：
粘贴相关日志。

要求：
- 先定位根因，不要直接猜测修复。
- 给出最小修复方案。
- 修复后补充或更新测试。
- 不要重构无关代码。
- 最后说明验证方式。
```

## 18. 文档生成提示词

```text
请为 Toide 当前实现生成或更新开发文档。

要求：
- 文档面向新加入项目的开发者。
- 说明项目结构。
- 说明如何构建客户端。
- 说明如何启动后端。
- 说明配置文件。
- 说明 REST API。
- 说明 WebSocket 事件协议。
- 说明常见开发问题。

输出文件建议：
- README.md
- docs/architecture.md
- docs/api.md
- docs/collaboration-protocol.md
- docs/development-setup.md

请不要写空泛介绍，要根据当前代码真实内容生成。
```

## 19. 实时协作调研提示词

```text
请为 Toide 的第三阶段实时多人编辑做技术调研和原型设计。

背景：
- Toide 是 C/S 架构。
- 服务端中心化协调。
- 当前已经有 projectId、filePath、baseVersion、clientId、CollaborationSession、WebSocket event。
- MVP 已经有文件版本和保存冲突检测。

请比较：
- OT
- CRDT

重点分析：
- 对 C++/Qt 客户端的实现难度。
- 对 Drogon 服务端的实现难度。
- 断线重连处理。
- 光标位置同步。
- 大文件性能。
- 与现有 file version 模型如何结合。

输出：
- 推荐方案。
- 数据结构设计。
- WebSocket 消息格式。
- 单文件协作编辑原型计划。
- 风险和测试方案。
```

## 20. 每次开发结束后的收尾提示词

```text
请对本次 Toide 开发改动做收尾检查。

请完成：
- 总结修改了哪些文件。
- 检查是否有未完成的半成品内容。
- 检查是否引入无关改动。
- 运行可用的构建或测试命令。
- 如果无法运行，说明原因。
- 给出下一步最合适的开发任务。

输出要简洁，但必须包含验证结果。
```
