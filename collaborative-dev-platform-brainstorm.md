# 协作开发平台头脑风暴

## 1. 产品定位

本项目定位为一个以 Qt 桌面客户端为主的协作开发 IDE 平台。整体采用 C/S 架构：

- Client：Qt/C++ 桌面 IDE，负责项目工作区、代码编辑、任务运行、Git 操作、协作状态展示。
- Server：后端服务，负责账号、团队、项目、权限、文件版本、协作会话、实时事件分发。

第一版目标不是一次性做成完整的 GitHub Codespaces 或 GitLab，而是先做出一个可运行、可扩展的“团队项目工作区”。多人协作能力需要从一开始规划进架构，但实现可以分阶段推进。

## 2. 核心用户场景

### 2.1 个人开发

- 用户打开 Qt 客户端。
- 创建或打开本地项目。
- 查看文件树。
- 编辑代码文件。
- 运行构建、测试或自定义任务。
- 查看 Git 状态并提交代码。

### 2.2 团队项目协作

- 用户登录账号。
- 加入团队或项目。
- 拉取项目工作区配置。
- 查看项目成员在线状态。
- 看到其他成员的文件修改事件。
- 编辑文件时获得冲突提醒。
- 保存文件后同步版本信息到服务端。

### 2.3 后续实时协作

- 多人同时打开同一个项目。
- 显示成员光标、选区和当前正在编辑的文件。
- 文件内容通过 WebSocket 实时同步。
- 使用 OT 或 CRDT 处理多人同时编辑冲突。
- 支持断线重连和状态恢复。

## 3. 功能规划

### 3.1 MVP 阶段

MVP 重点是“项目工作区 + 协作基础设施”。

客户端功能：

- 登录或本地用户配置。
- 创建、打开、关闭项目。
- 项目文件树。
- 基础代码编辑器。
- 文件保存。
- 最近文件列表。
- 集成任务运行面板。
- 构建、运行、测试命令配置。
- Git 状态查看。
- Git add、commit、branch、pull、push 的基础封装。
- 项目成员列表。
- 协作事件面板。

服务端功能：

- 用户账号。
- 团队和项目。
- 项目成员权限。
- 项目元数据。
- 文件版本记录。
- 文件修改事件。
- WebSocket 事件通道。
- 基础操作日志。

MVP 阶段可以暂时不做真正的多人同屏实时编辑，但数据模型、接口和通信协议需要提前预留。

### 3.2 第二阶段：半实时协作

- 在线成员状态。
- 当前正在查看或编辑的文件。
- 文件占用提示。
- 保存时版本冲突检测。
- 文件修改通知。
- 项目动态时间线。
- 文件评论和标注。
- 任务讨论。
- 简单代码评审流程。

这一阶段主要解决团队协作中的“互相知道对方在做什么”和“避免覆盖对方修改”。

### 3.3 第三阶段：实时协作编辑

- 多人同时编辑同一文件。
- 实时光标同步。
- 实时选区同步。
- 实时文本同步。
- 协作会话管理。
- 断线重连。
- 编辑历史回放。
- 冲突合并。

实时编辑算法建议后置调研：

- OT：传统协同编辑方案，适合中心化服务端协调。
- CRDT：更适合离线、弱网和去中心化场景，但实现复杂度更高。

如果项目前期以 C/S 架构和服务端中心协调为主，可以优先考虑 OT；如果后续希望支持离线编辑和复杂同步，再考虑 CRDT。

### 3.4 后续增强功能

- 插件系统。
- 代码补全。
- LSP 集成。
- AI 辅助开发。
- 远程开发容器。
- CI/CD 任务。
- Web 管理端。
- 项目文档。
- 权限审计。
- 企业部署。

## 4. 技术栈建议

### 4.1 客户端

推荐使用：

- C++20。
- Qt 6。
- Qt Widgets 作为第一版 UI 框架。
- CMake 作为构建系统。
- QNetworkAccessManager 处理 REST 请求。
- Qt WebSockets 处理实时事件。
- QProcess 运行构建、测试、终端命令。
- libgit2 集成 Git。
- SQLite 保存本地配置和缓存。

编辑器组件可选：

- QScintilla：成熟、容易集成，适合快速实现代码编辑器。
- KSyntaxHighlighting：适合语法高亮，但需要自己处理更多编辑器能力。
- 自研编辑器：可控性强，但成本高，不建议 MVP 阶段一开始就做。

建议 MVP 先使用 QScintilla 或成熟文本编辑组件，等核心平台跑通后再考虑自研编辑器。

### 4.2 后端

后端不强制必须 C++，但如果希望整体风格统一，可以优先考虑 C++ 后端。

C++ 后端可选：

- Drogon：性能好，支持 HTTP、WebSocket、ORM，适合 C++ Web 服务。
- oatpp：结构清晰，适合 REST API。
- Boost.Beast：底层能力强，但开发成本更高。

推荐优先使用 Drogon，因为它对 REST、WebSocket、数据库访问的支持更完整，适合作为协作平台后端。

后端基础设施：

- PostgreSQL：保存用户、项目、权限、文件版本、操作日志。
- Redis：保存在线状态、临时会话、WebSocket 节点状态。
- 本地磁盘或 MinIO：保存项目文件快照、附件、构建产物。
- Nginx：反向代理和 HTTPS。

### 4.3 通信协议

建议采用：

- REST：用于登录、项目管理、成员管理、权限管理、文件版本查询。
- WebSocket：用于在线状态、协作事件、文件变更通知、后续实时编辑。
- JSON：MVP 阶段足够简单，方便调试。
- Protobuf：后续实时协作高频消息较多时再考虑。

典型事件示例：

```json
{
  "type": "file.updated",
  "projectId": "project_001",
  "filePath": "src/main.cpp",
  "version": 12,
  "userId": "user_001",
  "timestamp": "2026-04-29T17:18:00+08:00"
}
```

## 5. 系统架构

### 5.1 总体架构

```text
Qt Desktop Client
  |-- REST API
  |-- WebSocket
  v
Backend Server
  |-- Auth Service
  |-- Project Service
  |-- File Version Service
  |-- Collaboration Service
  |-- Git Service
  v
PostgreSQL / Redis / File Storage
```

### 5.2 客户端模块

- AppCore：应用启动、配置、全局状态。
- Workspace：项目打开、关闭、工作区状态。
- FileExplorer：项目文件树。
- Editor：代码编辑器、标签页、保存、查找。
- TaskRunner：构建、运行、测试、自定义命令。
- GitPanel：Git 状态、提交、分支、同步。
- CollaborationPanel：在线成员、协作事件、文件修改提醒。
- NetworkClient：REST 和 WebSocket 通信。
- LocalCache：本地配置、最近项目、缓存数据。

### 5.3 服务端模块

- Auth：账号、登录、Token。
- Team：团队、成员。
- Project：项目、工作区配置。
- Permission：角色和权限。
- FileVersion：文件版本和修改记录。
- Collaboration：在线状态、WebSocket 会话、协作事件。
- GitIntegration：仓库信息、分支、提交记录。
- AuditLog：操作日志。

## 6. 数据模型草案

### 6.1 User

- id
- username
- email
- password_hash
- created_at
- last_login_at

### 6.2 Team

- id
- name
- owner_id
- created_at

### 6.3 Project

- id
- team_id
- name
- description
- repository_url
- workspace_config
- created_at

### 6.4 ProjectMember

- id
- project_id
- user_id
- role
- joined_at

角色建议：

- owner
- maintainer
- developer
- viewer

### 6.5 FileVersion

- id
- project_id
- file_path
- version
- content_hash
- updated_by
- updated_at

### 6.6 CollaborationSession

- id
- project_id
- user_id
- client_id
- current_file
- status
- connected_at
- last_seen_at

### 6.7 CollaborationEvent

- id
- project_id
- user_id
- event_type
- payload
- created_at

## 7. 协作设计重点

多人协作部分需要从第一版就定好边界：

- 每个项目有统一 project_id。
- 每个文件有 file_path 和 version。
- 客户端保存文件时必须携带基于哪个 version 修改。
- 服务端发现版本不一致时返回冲突。
- WebSocket 只传事件，不直接替代持久化接口。
- 文件内容保存仍通过明确的保存接口完成。
- 实时编辑后续可以复用 project_id、file_path、version 和 collaboration session。

MVP 保存文件流程：

```text
Client reads file version 10
Client edits file
Client saves with base version 10
Server checks latest version
If latest is 10:
  save content
  create version 11
  broadcast file.updated
If latest is not 10:
  reject save
  return conflict
```

这样即使第一版没有实时编辑，也能避免多人覆盖文件。

## 8. 推荐开发顺序

1. 搭建 Qt 客户端工程。
2. 实现主窗口、项目树、编辑器标签页。
3. 实现本地项目打开和文件编辑。
4. 加入任务运行面板。
5. 加入 Git 状态面板。
6. 搭建后端服务。
7. 实现用户、项目、成员、权限。
8. 实现文件版本 API。
9. 实现 WebSocket 协作事件。
10. 客户端接入项目成员和事件面板。
11. 实现保存冲突检测。
12. 再进入实时协作编辑调研和实现。

## 9. 风险和取舍

### 9.1 实时编辑复杂度高

不要第一天就实现完整多人实时编辑。建议先把版本、事件、在线状态、冲突检测做扎实。

### 9.2 自研编辑器成本高

IDE 的编辑器体验很复杂。MVP 优先使用成熟组件，把主要精力放在平台和协作模型上。

### 9.3 C++ 后端开发效率

C++ 后端性能好，但开发效率和生态不如 Go、Node.js、Python。由于你的核心熟悉方向是 Qt，建议客户端必须 C++/Qt，后端可以根据实际进度选择 Drogon 或其他更高效技术。

### 9.4 项目范围容易膨胀

协作开发平台天然容易扩展到代码托管、CI/CD、AI、远程容器、插件市场。第一版应该坚持项目工作区和协作基础，不要同时做所有功能。

## 10. 当前推荐结论

推荐路线：

- 架构：C/S 架构。
- 客户端：Qt 6 + C++20 + Qt Widgets。
- 后端：Drogon + PostgreSQL + Redis。
- 通信：REST + WebSocket。
- MVP：项目工作区、代码编辑、任务运行、Git 基础、成员列表、文件版本、协作事件。
- 多人协作：从第一版规划数据模型和协议，先做半实时协作，再做实时协作编辑。

一句话概括：

先做一个能独立使用的 Qt 桌面开发工作区，同时让它天生连接服务端、理解项目成员和文件版本，为后续真正的多人协同编辑打好基础。
