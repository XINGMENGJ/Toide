# Toide 服务端 — Linux 部署包说明

本目录用于在 **Linux（推荐 Ubuntu 22.04/24.04 或 Debian bookworm）** 上编译并运行 `toide_server`，**不包含 Qt 客户端**。代码与脚本与仓库同步维护。

## 目录结构（上传服务器后）

建议在服务器上保持如下布局（可用本仓库的打包脚本生成，见下文）：

```text
ToideLinuxServer/
  CMakeLists.txt      # 仅 add_subdirectory(server)
  server/             # 服务端源码与配置、迁移 SQL
  scripts/            # 依赖安装、编译 Drogon、编译/运行 Toide
  README.md           # 本说明（可选一并复制）
```

## 快速步骤

1. **安装系统依赖**（需 root）  
   ```bash
   sudo bash scripts/install-deps-debian.sh
   ```

2. **编译并安装 Drogon**（开启 MySQL ORM + Redis，`BUILD_CTL=OFF` 减少依赖）  
   默认安装到 `/usr/local`（需写权限时会自动使用 `sudo`）：  
   ```bash
   bash scripts/install-drogon.sh /usr/local
   ```  
   若安装到自定义目录（例如 `/opt/toide/env`）：  
   ```bash
   bash scripts/install-drogon.sh /opt/toide/env
   export CMAKE_PREFIX_PATH=/opt/toide/env
   export LD_LIBRARY_PATH=/opt/toide/env/lib:$LD_LIBRARY_PATH
   ```

3. **准备配置**  
   若尚无 `server/config/server.json`，可从示例复制并修改数据库、Redis 与监听地址：  
   ```bash
   cp -n server/config/server.json.example server/config/server.json
   nano server/config/server.json
   ```  
   在 MySQL 中建库 `toide`、用户与权限后，可执行迁移（按需）：  
   ```bash
   mysql -u toide -p toide < server/migrations/001_users.sql
   mysql -u toide -p toide < server/migrations/002_workspace_files.sql
   ```

4. **编译 Toide 服务端**（在部署包**根目录**，即包含 `CMakeLists.txt` 与 `server/` 的目录）  
   ```bash
   chmod +x scripts/*.sh
   export CMAKE_PREFIX_PATH=/usr/local   # 若 Drogon 在自定义前缀，改为该路径（多个用英文冒号 : 分隔）
   bash scripts/build-toide-server.sh
   ```

5. **运行**  
   ```bash
   bash scripts/run-toide-server.sh
   ```  
   默认监听见 `server.json`（示例为 `0.0.0.0:8848`）。健康检查：  
   `curl -s http://127.0.0.1:8848/api/health`  

   可选：指定配置文件路径  
   ```bash
   export TOIDE_SERVER_CONFIG=/绝对路径/server/config/server.json
   bash scripts/run-toide-server.sh
   ```

6. **生产环境**  
   - 建议使用 **systemd** 或 **进程管理器** 守护运行，并将工作目录设为部署包根目录。  
   - 打开防火墙中对应 TCP 端口；Redis、MySQL 建议使用独立实例并配置强密码。  
   - 若 Drogon 装在非系统路径，请在服务单元里设置 `Environment=LD_LIBRARY_PATH=/opt/toide/env/lib`。

## 在 Windows 上生成分发包（便于上传）

在已克隆的仓库**任意位置**执行均可：

```powershell
powershell -ExecutionPolicy Bypass -File deploy/linux-server/pack-for-linux.ps1
```

将生成 `dist\ToideLinuxServer\`，把整个文件夹打包为 zip/tar.gz 上传到服务器即可（**不要**提交 `server/config/server.json` 中的生产口令到公开仓库；上传前可只保留 `server.json.example`，在服务器上再复制修改）。

## 故障排除

- **CMake 版本过低**：`install-deps-debian.sh` 在 Ubuntu 上会尝试添加 Kitware 源；也可自行安装 CMake 3.24+。  
- **找不到 Drogon**：设置 `CMAKE_PREFIX_PATH` 为 Drogon 的 `CMAKE_INSTALL_PREFIX`，重新执行 `build-toide-server.sh`。  
- **运行报找不到 .so**：设置 `LD_LIBRARY_PATH` 包含 Drogon 安装前缀下的 `lib`，或执行 `sudo ldconfig /你的前缀/lib`。  
- **Drogon 浅克隆子模块失败**：删除 `.third_party/drogon-src` 后改用完整克隆：  
  `git clone --recurse-submodules https://github.com/drogonframework/drogon.git` 再按 Drogon 文档编译安装。

## 与仓库完整克隆的关系

若服务器上直接 `git clone` 整个 Toide 仓库，也可只使用本目录脚本：将 `deploy/linux-server/scripts` 与根目录的 `server` 搭配使用，或复制本目录的 `CMakeLists.txt` 到仅含 `server` 的同级目录后同样按上述步骤操作。
