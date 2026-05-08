# Toide

轻量级协作向 IDE 客户端 + 配套后端：**Qt 6（Widgets）** 桌面端，**Drogon** HTTP/WebSocket 服务端，MySQL / Redis 作持久与协作状态（可选，缺省时服务端有降级行为）。

## 功能概览

- **工作区**：打开文件夹、文件树、多标签编辑器、保存与快捷键
- **本地任务**：读取 `.toide/tasks.json`，`QProcess` 运行构建/诊断任务，解析 g++/MSVC 风格诊断并支持点击跳转
- **工作区语法检查**：底栏「编译」对指定源码做 `g++`/`gcc` `-fsyntax-only`
- **Git**：状态面板（分支、改动分组、复制状态、打开终端）
- **网络与协作**：服务端健康检查；在已安装 **Qt WebSockets** 的构建下，协作面板可连接项目 WebSocket（在线成员、活动、心跳与重连等）

## 仓库结构

| 路径 | 说明 |
|------|------|
| `client/` | Qt 客户端源码与 CMake、`CTest` 测试 |
| `server/` | Drogon 服务端与健康检查、认证、工作区文件、协作 WS 等 |
| `examples/default-workspace/` | 默认示例工作区（启动时可自动打开） |
| `deploy/linux-server/` | Linux 服务端独立打包说明与脚本 |
| `scripts/` | 打包、图标生成、后端/Redis 辅助脚本 |

内部开发与联调说明见 `development-context.md`（路径、依赖、约定）。

## 依赖

- **客户端**：Qt **6.11**（推荐）或 6.x，`mingw_64` 或 MSVC；协作功能需安装 **Qt WebSockets** 组件
- **服务端**：C++20、Drogon、MySQL（或 MariaDB 客户端库）、Redis（可选）、CMake ≥ 3.24

## 构建客户端（CMake）

```bash
cmake -G "MinGW Makefiles" -B build-mingw -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=<Qt6>/mingw_64 ^
  -DCMAKE_CXX_COMPILER=<Qt-Tools>/mingw1310_64/bin/g++.exe
cmake --build build-mingw --parallel
ctest --test-dir build-mingw --output-on-failure
```

Windows 下打包发布目录（依赖 `windeployqt`）：

```powershell
.\scripts\package-client.ps1 -BuildDir build-mingw -QtDir <Qt6>/mingw_64 -OutputDir dist/ToideClient
```

自根目录 PNG 生成 `client/resources/app.ico` 并重编、打包，可用 `scripts/build-client-package.ps1`（默认优先 `三代图标.png`）。

## 构建客户端（qmake / Qt Creator）

打开根目录 `Toide.pro`，选择与本机一致的 Kit，**运行 qmake** 后构建。若协作不可用，请在 Maintenance Tool 中为该 Qt 安装 **WebSockets** 后重新 qmake。

## 构建服务端

在配置好 Drogon、MySQL 客户端库、可选 hiredis 的前提下，于 `server/config/` 参考 `server.json.example` 编写 `server.json`，再与顶层 `CMakeLists.txt` 一同配置构建。详见 `scripts/toide-server-env.cmd`、`deploy/linux-server/README.md`。

## 许可

未随仓库附带统一开源许可证；使用前请与维护者确认使用范围。
