# Toide

轻量级协作向 IDE：**Qt 6 桌面客户端** + **Drogon HTTP/WebSocket 服务端**。支持工作区编辑、本地任务、Git 状态、登录注册与实时协作（需 MySQL + 可选 Redis）。

## 功能概览

- 工作区：文件树、多标签编辑器、保存与快捷键
- 本地任务：`.toide/tasks.json` 驱动 `QProcess` 构建/诊断
- Git：分支与改动分组、复制状态、打开终端
- 网络协作：健康检查、WebSocket 在线成员与文件同步（需 Qt WebSockets 组件）

## 仓库结构

| 路径 | 说明 |
|------|------|
| `client/` | Qt 客户端源码与测试 |
| `server/` | Drogon 服务端、配置样例、SQL 迁移 |
| `examples/default-workspace/` | 启动时默认打开的示例工作区 |
| `deploy/linux-server/` | Linux 服务端打包与安装脚本 |
| `scripts/` | 构建、运行、依赖安装、Redis 辅助脚本 |
| `tools/redis-windows/` | Windows 本地 Redis 配置（二进制由脚本下载） |

## 从零部署（Windows，推荐）

### 1. 安装基础工具

| 组件 | 版本要求 | 说明 |
|------|----------|------|
| [Qt](https://www.qt.io/download) | 6.11（或 6.x） | 选择 **MinGW 64-bit**；协作功能需额外安装 **Qt WebSockets** |
| [Git](https://git-scm.com/) | 任意较新版本 | 克隆仓库与编译 Drogon 依赖 |
| [MySQL](https://dev.mysql.com/downloads/mysql/) | 5.7+ 或 8.x | 本地 `127.0.0.1:3306` |
| Redis（可选） | 任意 | 协作状态持久化；不可用时会回退内存模式 |

克隆仓库：

```powershell
git clone https://github.com/XINGMENGJ/Toide.git
cd Toide
```

配置本机 Qt 路径（二选一）：

```powershell
# 方式 A：复制示例并编辑
copy toide-env.local.cmd.example toide-env.local.cmd
notepad toide-env.local.cmd

# 方式 B：临时环境变量
$env:TOIDE_QT_DIR = "C:\Qt\6.11.0\mingw_64"
$env:TOIDE_QT_TOOLS = "C:\Qt\Tools"
```

验证 Qt 环境：

```cmd
call qt6.7-env.cmd
g++ --version
cmake --version
```

### 2. 准备 MySQL

以管理员身份初始化数据库与用户：

```cmd
mysql -u root -p < scripts\init-mysql.sql
mysql -u toide toide < server\migrations\001_users.sql
mysql -u toide toide < server\migrations\002_workspace_files.sql
mysql -u toide toide < server\migrations\003_workspaces.sql
```

复制并检查服务端配置（默认连接 `toide`/`toide`、空密码、端口 `8848`）：

```cmd
copy server\config\server.json.example server\config\server.json
```

### 3. 安装服务端原生依赖（Drogon 等）

在已加载 Qt/MinGW 环境的 PowerShell 中执行（安装到 `%USERPROFILE%\local`，约 15–30 分钟）：

```powershell
call qt6.7-env.cmd
powershell -ExecutionPolicy Bypass -File scripts\install-windows-server-deps.ps1
```

脚本会编译并安装：zlib、jsoncpp、hiredis、MariaDB Connector/C、Drogon（开启 MySQL ORM + Redis）。`scripts\toide-server-env.cmd` 已指向该前缀。

### 4. 启动 Redis（可选，推荐）

```powershell
# 下载 Windows 版 Redis 并启动（127.0.0.1:6379）
powershell -ExecutionPolicy Bypass -File scripts\bootstrap-redis-windows.ps1
scripts\start-redis-windows.cmd
```

也可使用 Docker：`docker run -d --rm --name toide-redis -p 6379:6379 redis:7-alpine`

### 5. 一键构建、启动后端、打包客户端

```cmd
scripts\one-click-build-run-package.cmd
```

或分步执行：

```cmd
scripts\build-toide-all.cmd          :: 构建 toide_server + toide_client
scripts\start-backend-window.cmd     :: 新窗口启动后端
powershell -File scripts\package-client.ps1 -QtDir "%TOIDE_QT_DIR%"
```

构建产物：

| 输出 | 路径 |
|------|------|
| 后端 | `build-mingw\server\toide_server.exe` |
| 客户端（开发） | `build-mingw\client\toide_client.exe` |
| 客户端（发布包） | `dist\ToideClient\Toide.exe` |

健康检查：`http://127.0.0.1:8848/api/health`

运行发布包：先确保后端已启动，再双击 `dist\ToideClient\Toide.exe`。

### 6. 仅客户端开发（不编服务端）

若不需要 Drogon，可只构建客户端：

```cmd
call qt6.7-env.cmd
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=%QTDIR% ^
  -DCMAKE_CXX_COMPILER=%TOIDE_QT_TOOLS%\mingw1310_64\bin\g++.exe
cmake --build build-mingw --target toide_client --parallel
ctest --test-dir build-mingw --output-on-failure
```

也可用 Qt Creator 打开根目录 `Toide.pro`。

## 从零部署（Linux 服务端）

适用于在 Ubuntu/Debian 上仅部署 `toide_server`（不含 Qt 客户端）。

```bash
# 在 Windows 上生成分发包（可选）
powershell -ExecutionPolicy Bypass -File deploy/linux-server/pack-for-linux.ps1
# 上传 dist/ToideLinuxServer/ 到服务器

# 在 Linux 服务器上
sudo bash scripts/install-deps-debian.sh
bash scripts/install-drogon.sh /usr/local
cp -n server/config/server.json.example server/config/server.json
# 编辑 server.json 中的 MySQL/Redis 地址
mysql -u toide -p toide < server/migrations/001_users.sql
mysql -u toide -p toide < server/migrations/002_workspace_files.sql
mysql -u toide -p toide < server/migrations/003_workspaces.sql
export CMAKE_PREFIX_PATH=/usr/local
bash scripts/build-toide-server.sh
bash scripts/run-toide-server.sh
```

## 配置说明

`server/config/server.json`（勿提交含生产密码的副本）：

- `listeners`：HTTP/WebSocket 监听地址，默认 `0.0.0.0:8848`
- `db_clients`：MySQL 连接（用户认证、工作区文件版本）
- `redis_clients`：协作在线状态与光标（可选）

客户端服务端地址保存在用户配置目录的 `toide-client.ini`（默认 `http://127.0.0.1:8848`），可在协作面板修改。

## 常用脚本

| 脚本 | 用途 |
|------|------|
| `qt6.7-env.cmd` | 加载 Qt + MinGW + CMake 到 PATH |
| `scripts\build-toide-all.cmd` | 构建服务端与客户端 |
| `scripts\build-toide-server.cmd` | 仅构建服务端 |
| `scripts\run-backend.cmd` | 前台运行后端 |
| `scripts\start-toide-dev.ps1` | 开发模式：可选 Redis + 构建 + 启动后端 |
| `scripts\package-client.ps1` | `windeployqt` 打包到 `dist\ToideClient` |
| `scripts\install-windows-server-deps.ps1` | Windows 编译安装 Drogon 依赖链 |
| `scripts\bootstrap-redis-windows.ps1` | 下载 Windows Redis |

## 故障排除

| 现象 | 处理 |
|------|------|
| `Qt MinGW not found` | 创建 `toide-env.local.cmd` 或设置 `TOIDE_QT_DIR` |
| `Drogon not found` | 运行 `install-windows-server-deps.ps1`，确认 `toide-server-env.cmd` 中的前缀 |
| 启动后端报 `libmariadb.dll` 缺失 | 将 `%USERPROFILE%\local\mariadb-connector-c\lib\mariadb` 加入 PATH |
| `DbClientManagerSkipped` | Drogon 未开启 `BUILD_ORM`/`BUILD_MYSQL`，需重装 Drogon |
| 协作 WebSocket 不可用 | 在 Qt Maintenance Tool 中为当前 Kit 安装 **WebSockets**，重新 CMake/qmake |
| MySQL 1045 登录失败 | 检查 `server.json` 用户密码与 `scripts\init-mysql.sql` 是否已执行 |
| Redis 未连接 | 启动 Redis 或忽略（服务端自动回退内存协作） |

## 许可

未随仓库附带统一开源许可证；使用前请与维护者确认使用范围。
