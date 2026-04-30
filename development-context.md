# Toide 开发上下文

## 协作约定

- **持久路线**：服务端数据层以 **MySQL + Redis** 为主；若考虑改为 SQLite/单库-only 等架构方向，**必须先与用户确认**，不得在未确认时自行替换。
- 本仓库的**开发上下文以本文档为准**：完成与本机构建、依赖路径、Drogon/MySQL 相关的重要变更后，应同步更新本节与「本机 Drogon / MySQL 客户端」段落。
- 用户说“提交”时，表示本地 `git commit` 后还要 `git push` 到 GitHub。
- 用户说“可以，继续开发”时，继续下一个小功能前，需要更新本文档，记录当前上下文。
- `Toide.pro` 是 Qt Creator/qmake 的手动编译入口，可以随代码变化同步维护。
- 后续允许使用 C++20 内容，但必须同步维护 `Toide.pro`，让 qmake/Qt Creator 使用 `CONFIG += c++20` 生成当前 Kit 兼容的 C++20 编译参数。
- 每次功能完成后优先运行：
  - CMake/CTest 测试。
  - qmake 构建。
  - 相关文件 lints。

## 当前技术栈

- 客户端：Qt 6.11 + C++20/Qt Widgets（当前可用 Kit 为 **E:/QT/QTN/6.11.0/mingw_64**，并已确认含 `Qt6WebSockets`；旧版 Qt 6.7 可保留但不作为协作功能开发 Kit）。
- 构建：
  - CMake：主自动化构建和测试。
  - qmake：`Toide.pro`，供 Qt Creator 手动打开编译；MinGW 依赖 `CONFIG += c++20` 生成兼容标准参数，MSVC 显式使用 `/std:c++20`。
- 仓库：GitHub `XINGMENGJ/Toide`。
- 服务端：`server/config/server.json` 已配置 Drogon `db_clients`（MySQL `toide`）与 `redis_clients`（`default`）。首次注册/登录会在 MySQL 中 `CREATE TABLE IF NOT EXISTS users`（与 `server/migrations/001_users.sql` 一致）；**AuthService** 优先走 MySQL，失败时回退内存用户表。WebSocket 协作在 **Redis 可用** 时写入 `toide:presence:project:*`、`toide:session:{clientId}`、`toide:cursor:...`、`toide:softlock:...`（与开发文档 §5.2 键名语义一致）；Redis/MySQL 不可用时仍仅用内存房间，行为与此前兼容。
- 本机 Drogon 已安装在 `%USERPROFILE%\local\drogon`（示例：`C:\Users\fbyf7\local\drogon`）。源码与构建目录示例：`%USERPROFILE%\drogon`，MinGW 构建目录 `build-mingw-mysql`。安装 Drogon 时须 **`BUILD_ORM=ON`**、**`BUILD_MYSQL=ON`**、**`BUILD_REDIS=ON`**；若 **`BUILD_ORM=OFF`** 会导致 `DbClientManagerSkipped`、等同于未编进数据库支持。可关 **`BUILD_EXAMPLES`**、**`BUILD_SQLITE`**、**`BUILD_POSTGRESQL`** 以缩短构建；**`BUILD_CTL=OFF`** 可避免 `drogon_ctl` 在 MinGW 下因 Hiredis 传递依赖不完整而链接失败。
- **MySQL 客户端库**：Oracle MySQL 5.7 自带的 `libmysql` **不含** Drogon 所需的 MariaDB 异步 API（如 `mysql_real_connect_start`）。链接目标应为本地编译的 **[MariaDB Connector/C](https://mariadb.com/kb/en/mariadb-connector-c/)**（安装前缀示例：`%USERPROFILE%\local\mariadb-connector-c`），**仍连接本机 MySQL 5.7 服务**（协议兼容）。本机在 Drogon 源码侧曾调整 `cmake_modules/FindMySQL.cmake`、`cmake_modules/FindHiredis.cmake`（MinGW 下 hiredis 避免强依赖 zstd/curl）；`orm_lib/src/mysql_impl/MysqlConnection.cc` 在 `mysql_init` 后关闭强制 SSL 校验，避免 MariaDB 客户端连 **无 SSL 的 MySQL 5.7** 时报 “SSL is required…”。
- **Redis 客户端**：hiredis 安装前缀示例：`%USERPROFILE%\local\hiredis`。
- CMake 配置 `toide_server` 时 `CMAKE_PREFIX_PATH` 除 Qt 与 Drogon 外，建议包含：`mariadb-connector-c`、`hiredis`、`jsoncpp`、`zlib`（与 `scripts\toide-server-env.cmd` 一致）。**运行时 PATH** 需包含 **`%USERPROFILE%\local\mariadb-connector-c\lib\mariadb`**（`libmariadb.dll` 与 `plugin\` 下的认证插件）。
- **本机快速校验**（2026-04-30）：将上述 `lib\mariadb` 加入 PATH 后启动 `build-mingw\server\toide_server.exe`，`GET http://127.0.0.1:8848/api/health` 返回 `{"status":"ok",...}` 即进程与监听正常；若 MySQL/Redis 仍连不上，以 Drogon 日志与 `server.json` 为准逐项排查。
- 后端启动脚本：`scripts\build-toide-server.cmd` 和 `scripts\run-backend.cmd` 默认使用 **`build-mingw\server\toide_server.exe`**（Qt 6.11 + MinGW 13 + `C:\Users\fbyf7\local\drogon`）。旧目录 **`build-server\`** 可能是历史 Qt 6.7/MinGW 11 构建，若误运行可能出现 `DbClientManagerSkipped` / “No database is supported by drogon”；除非明确清理重建，否则不要优先使用它。
- MySQL 服务为 `MySQL57`，监听 `127.0.0.1:3306`；root 管理员密码以本地口令为准（此前文档误记拼写导致 1045）。已用 root 在本机验证可登录；并已建库 `toide`、用户 `toide`@`localhost`（空密码，与 `server/config/server.json` 一致），对 `toide.*` 有全部权限。
- Redis：`server.json` 已配置 `redis_clients`；本机 **若已启动 Redis**（例如监听 6379），协作状态会写入文档约定前缀的 key；未启动或连接失败时仅使用内存房间，不影响联调。
- 协作稳定性约定：REST/MySQL 文件版本是保存同步的权威来源；WebSocket `file.updated` 若不含 `content` 只作为活动/版本提示，**不得覆盖远端编辑器内容**。实时 `editor.patch` 仅在消息含 `content` 时应用正文；超大正文会被省略以避免断开。远端 `editor.cursor`/`editor.soft_lock` 会在编辑器中以行高亮和 tooltip 提示占用位置。协作频道会发送 `heartbeat`，意外断开后自动重连，用户手动断开时不自动重连。
- 客户端打包脚本：`scripts/package-client.ps1`，默认把 `build-mingw/client/toide_client.exe` 通过 Qt 6.11 的 `windeployqt` 封装到 `dist/ToideClient/Toide.exe`。

## 已完成内容

- 项目规划文档和 AI 开发提示词。
- Linux 服务端独立部署包：`deploy/linux-server/`（`README.md`、`scripts/*.sh`、顶层 `CMakeLists.txt` 仅构建 server）；Windows 下执行 `deploy/linux-server/pack-for-linux.ps1` 生成 `dist/ToideLinuxServer/` 便于上传服务器。
- Qt 6.7 qmake 手动构建文件和环境脚本。
- 本地项目打开和文件树。
- 文件树支持右键新建文件和新建文件夹；新建文件成功后会立即打开编辑器。
- 基础编辑器标签页。
- 当前文件保存动作和 `Ctrl+S`。
- 最近项目记录。
- 任务配置解析。
- 任务执行请求解析。
- `QProcess` 任务执行器。
- 任务运行面板起步版，可读取 `.toide/tasks.json` 并显示任务列表。
- `Toide.pro` 已配置 C++20：MinGW 使用 qmake 的 `CONFIG += c++20` 映射，MSVC 使用 `/std:c++20`。
- Qt Creator 兼容性修复已提交：移除 `std::optional` 依赖、移除生产代码 designated initializer、替换 `QProcess::startCommand()`。
- 默认示例工作区位于 `examples/default-workspace`，启动时会自动打开，用来展示文件树、编辑器、保存和任务面板。
- 为兼容 Qt Creator shadow build，CMake 和 `Toide.pro` 都定义了 `TOIDE_SOURCE_DIR`，启动时会从源码根目录查找默认示例工作区。
- Tasks 面板支持运行状态展示：空闲显示 `Idle`，运行中显示当前任务名。
- Tasks 面板支持 Stop 按钮，可中止正在运行的 `QProcess` 任务。
- 默认示例工作区提供 `Build Example` 和 `Run Example` 任务，可演示基础 C++ 编译/运行流程。
- Tasks 面板在任务自然结束后显示 `Succeeded` 或 `Failed: exit code N`，让编译结果更清晰。
- 编译诊断解析器可识别 g++ 和 MSVC 风格的 `error` / `warning` / `note` 输出，提取文件、行列号、级别和消息。
- 默认示例工作区提供 `include/` + `src/` 多文件示例与 `diagnostics/diagnostic_demo.cpp`，以及 `Build Diagnostics Demo` 任务，可故意触发编译错误来验证诊断功能。
- 默认示例工作区的编译任务会先加载 `qt6.7-env.cmd`，避免程序内 PATH 缺少 `g++` 或调用旧编译器。
- 默认示例工作区的编译任务不再使用 `if not exist build mkdir build && ...`，避免 `build` 目录已存在时跳过真正的编译命令。
- `Run Example` 也会加载 `qt6.7-env.cmd`，避免运行示例 exe 时缺少 MinGW 运行库 DLL；如果 `build\\hello_toide.exe` 缺失则提示先运行 `Build Example`。
- Tasks 面板会在任务结束后解析输出中的编译诊断，并追加 `Diagnostics:` 摘要。
- 编辑器区域支持 `openFileAt(file, line, column)`，可打开文件并移动光标到指定行列。
- Tasks 面板的诊断摘要项支持点击，点击后会通过主窗口打开对应文件并跳转到行列位置。
- Git 标签页起步版可在打开工作区时显示 `git status --short --branch`，非 Git 目录会显示提示。
- Git 状态面板会把 `git status --short --branch` 整理成 Branch、Staged、Unstaged、Untracked 分组。
- Git 状态面板会显示刷新结果：成功、非 Git 仓库、超时或未打开工作区。
- Git 状态面板刷新结果提示已提交并推送：`9fcb740 feat: show git refresh status`。
- Git 状态面板提供 `Copy status` 按钮，可复制当前整理后的状态文本到剪贴板。
- Git 状态面板复制状态功能已提交并推送：`d7ec724 feat: copy git status output`。
- Git 状态面板提供 `Open terminal` 按钮，会请求主窗口在当前工作区打开系统终端。
- Git 状态面板打开终端功能已提交并推送：`abcca3d feat: open terminal from git panel`。
- Git 状态面板在工作区无 staged、unstaged、untracked 改动时会显示 `Working tree clean.`。
- Git 状态面板 clean 状态提示已提交并推送：`5cccdd1 feat: show clean git status`。
- Git 状态面板在存在改动时会显示 staged、unstaged、untracked 数量摘要。
- Git 状态面板数量摘要已提交并推送：`b4b3610 feat: summarize git status counts`。
- Git 状态面板点击 `Copy status` 后会显示 `Copied status` 反馈。
- Git 状态面板复制反馈已提交并推送：`bbb5305 feat: show git status copy feedback`。
- Git 状态面板点击 `Open terminal` 后会显示 `Opening terminal` 反馈。
- Git 状态面板打开终端反馈已提交并推送：`f0b3969 feat: show git terminal opening feedback`。
- Git 状态面板无状态内容时点击 `Copy status` 会显示 `No status to copy`，并不会覆盖现有剪贴板内容。
- Git 状态解析里的 `QString::split(..., SkipEmptyParts)` 已按 Qt 主版本兼容，避免旧 Kit 下 `Qt::SkipEmptyParts` 不存在导致编译失败。
- Git 状态面板空状态复制和 `SkipEmptyParts` 兼容修复已提交并推送：`aa6dd71 fix: handle empty git status copy`。
- Git 状态面板提供 `Copy branch` 按钮，可复制当前分支名，并在 Windows 剪贴板短暂占用时进行短重试。
- Git 状态面板复制分支名功能已提交并推送：`4fe694b feat: copy git branch name`。
- Git 状态面板在未解析到分支名时会禁用 `Copy branch`，成功加载分支后再启用。
- 网络协作主线已启动：后端数据库路线从 PostgreSQL 调整为 MySQL + Redis。
- 新增 `server/` 后端骨架：包含可测试的 `toide_server_core`、health 响应生成、配置样例和 Drogon 控制器入口。
- Drogon 已可被 CMake 找到；`toide_server` 目标可以在 MinGW + Qt 6.11 + `C:\Users\fbyf7\local\drogon` 前缀下构建。
- 安装 Drogon 时，`toide_server` 增加 `CollaborationWsController`：WebSocket 路径正则 `^/ws/projects/([^/]+)$`，握手后发送 `server.welcome`，同项目房间广播 `presence.user_joined` / `presence.user_left`；支持客户端 JSON `heartbeat` → `heartbeat.ack`，以及 `presence.join`、`presence.current_file` 向其他连接转发（用户占位 `anonymous`，`?token=` 校验与真实用户身份待接）。
- 客户端新增 `NetworkClient`，可请求服务端 `/api/health` 并通过 `healthChecked` 信号报告 Online/Offline 状态。
- 主窗口右侧协作面板从占位列表升级为 `CollaborationPanelWidget`，提供 `Check server` 按钮并显示服务端连接状态；健康检查请求 `NetworkClient::checkHealth`，URL 为设置中的服务端基址 + `/api/health`。
- 客户端 `ServerEndpointSettings` 将协作服务端基址（默认 `http://127.0.0.1:8848`）持久化到 `QStandardPaths::AppConfigLocation` 下的 `toide-client.ini`（键 `network/serverBaseUrl`）；协作面板提供可编辑行，失焦与点击检查时写回并规范化 URL；单测可用临时 ini 路径隔离。
- 客户端 CMake 在 MSVC 上对 `toide_client_core` 使用 `/Zc:__cplusplus`，以满足 Qt 头文件对标准宏的要求；若 Qt Kit 与 MSVC 不匹配（例如引用 MinGW 版 Qt 却用 VS 生成器），链接会失败，此时应改用 MinGW + `MinGW Makefiles`（例如配合仓库内 `qt6.7-env.cmd`）或改为 MSVC 对应的 Qt 安装。
- 协作实时通道：`CollaborationWebSocketClient`（`QWebSocket`）与 `buildCollaborationWebSocketUrl`（HTTP 基址映射为 `ws`/`wss`，路径 `/ws/projects/{id}`，可选 `token` 查询参数），配套 CTest 在本地 `QWebSocketServer` 上做 echo 验证。**当前推荐 Qt 6.11 的 `mingw_64`**：若其中已安装 **Qt WebSockets**（存在 `lib/cmake/Qt6WebSockets`），CMake 会编入该类并运行 `toide_collaboration_websocket_client_test`；仅装 **6.7** 且无 WebSockets 时仍会跳过。qmake 侧用 `qtHaveModule(websockets)` 与之一致；**定义 `TOIDE_HAVE_QT_WEBSOCKETS`** 时协作面板含 **Connect / Disconnect collaboration channel**，按当前工作区文件夹名作为 `projectId` 连接；未定义时显示未编入说明并隐藏按钮。

## 最近一次问题

Qt Creator 页面编译失败，错误集中在：

- `std::optional` 不可用。
- `QProcess::startCommand()` 不存在。
- C++20 designated initializer 报警或失败。

处理方式：

- `TaskConfig::loadFromFile` 改为返回项目内的 `TaskConfigLoadResult`。
- 移除生产代码里的 C++20 designated initializer。
- `QProcess::startCommand()` 改为 Windows 使用 `cmd.exe /C`，非 Windows 使用 `/bin/sh -c`。
- `Qt::SkipEmptyParts` 在部分 Qt Kit 下不可用，改为 Qt 5 使用 `QString::SkipEmptyParts`、Qt 6 使用 `Qt::SkipEmptyParts`。

- 客户端（2026-05-01）：底栏「编译」页 + 主工具栏可对当前工作区 `include/` 与 `src/` 下的 `.h`/`.c`/`.cpp` 等做本机 `g++`/`gcc` 的 `-fsyntax-only` 语法检查（需 PATH 中可用编译器）；协作面板「在线成员」使用服务端 roster / `presence.current_file_changed` 中的 `username` 显示昵称；主窗口构造末尾调用 `clearAuthSession()`，启动时默认未登录（仍保留服务器地址等设置）；「帮助 → 更新日志」通过 Qt 资源 `:/toide/app_changelog.txt`（源文件 `client/resources/app_changelog.txt`）在程序内展示变更摘要；Git 页补充说明文案与工具栏布局（刷新/终端/复制）更易理解。

## 当前未提交改动

- 工作区与远程差异请以 `git status` / ` git log origin/main..HEAD` 为准。

## 下一步建议

认证已可走 MySQL（失败回退内存）；Redis 可用时协作键已写入文档约定前缀。后续以小步迭代为主。若执行 `cmake --install` 时 **误写 `$prefix` 字面量**，可能在仓库根下生成错误目录 **`$prefix\`**，应删除并改用 **`--prefix C:/Users/.../local/...`** 形式。

优先保持 Qt 6.11 MinGW + Drogon（ORM+MySQL+Redis）构建绿色。
