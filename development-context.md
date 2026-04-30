# Toide 开发上下文

## 协作约定

- 用户说“提交”时，表示本地 `git commit` 后还要 `git push` 到 GitHub。
- 用户说“可以，继续开发”时，继续下一个小功能前，需要更新本文档，记录当前上下文。
- `Toide.pro` 是 Qt Creator/qmake 的手动编译入口，可以随代码变化同步维护。
- 后续允许使用 C++20 内容，但必须同步维护 `Toide.pro`，让 qmake/Qt Creator 使用 `CONFIG += c++20` 生成当前 Kit 兼容的 C++20 编译参数。
- 每次功能完成后优先运行：
  - CMake/CTest 测试。
  - qmake 构建。
  - 相关文件 lints。

## 当前技术栈

- 客户端：Qt 6.11 + C++20/Qt Widgets（仓库内 `qt6.7-env.cmd` 已为历史文件名，实际加载 **Qt 6.11.0 MinGW 64-bit** 与 **Tools/mingw1310_64**；机器上仍可同时保留旧版 Qt 6.7 安装）。
- 构建：
  - CMake：主自动化构建和测试。
  - qmake：`Toide.pro`，供 Qt Creator 手动打开编译；MinGW 依赖 `CONFIG += c++20` 生成兼容标准参数，MSVC 显式使用 `/std:c++20`。
- 仓库：GitHub `XINGMENGJ/Toide`。
- 服务端规划：Drogon + MySQL + Redis，REST 用于业务接口，WebSocket 用于协作事件。

## 已完成内容

- 项目规划文档和 AI 开发提示词。
- Qt 客户端主窗口骨架。
- Qt 6.7 qmake 手动构建文件和环境脚本。
- 本地项目打开和文件树。
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
- 默认示例工作区提供 `diagnostic_demo.cpp` 和 `Build Diagnostics Demo` 任务，可故意触发编译错误来验证诊断功能。
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
- 当前本机未发现 Drogon 包，CMake 会先构建 server core 和测试；安装 Drogon 后会自动构建 `toide_server` 可执行文件。
- 客户端新增 `NetworkClient`，可请求服务端 `/api/health` 并通过 `healthChecked` 信号报告 Online/Offline 状态。
- 主窗口右侧协作面板从占位列表升级为 `CollaborationPanelWidget`，提供 `Check server` 按钮并显示服务端连接状态；健康检查请求 `NetworkClient::checkHealth`，URL 为设置中的服务端基址 + `/api/health`。
- 客户端 `ServerEndpointSettings` 将协作服务端基址（默认 `http://127.0.0.1:8848`）持久化到 `QStandardPaths::AppConfigLocation` 下的 `toide-client.ini`（键 `network/serverBaseUrl`）；协作面板提供可编辑行，失焦与点击检查时写回并规范化 URL；单测可用临时 ini 路径隔离。
- 客户端 CMake 在 MSVC 上对 `toide_client_core` 使用 `/Zc:__cplusplus`，以满足 Qt 头文件对标准宏的要求；若 Qt Kit 与 MSVC 不匹配（例如引用 MinGW 版 Qt 却用 VS 生成器），链接会失败，此时应改用 MinGW + `MinGW Makefiles`（例如配合仓库内 `qt6.7-env.cmd`）或改为 MSVC 对应的 Qt 安装。
- 协作实时通道：`CollaborationWebSocketClient`（`QWebSocket`）与 `buildCollaborationWebSocketUrl`（HTTP 基址映射为 `ws`/`wss`，路径 `/ws/projects/{id}`，可选 `token` 查询参数），配套 CTest 在本地 `QWebSocketServer` 上做 echo 验证。**当前推荐 Qt 6.11 的 `mingw_64`**：若其中已安装 **Qt WebSockets**（存在 `lib/cmake/Qt6WebSockets`），CMake 会编入该类并运行 `toide_collaboration_websocket_client_test`；仅装 **6.7** 且无 WebSockets 时仍会跳过。qmake 侧用 `qtHaveModule(websockets)` 与之一致。

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

## 当前未提交改动

- 工作区与远程差异请以 `git status` / ` git log origin/main..HEAD` 为准。

## 下一步建议

继续实现服务端 Drogon WebSocket 房间与认证（与 `collaborative-dev-platform-development-guide.md` §7 对齐），并把客户端协作面板接入 `CollaborationWebSocketClient`。
