# Toide 开发上下文

## 协作约定

- 用户说“提交”时，表示本地 `git commit` 后还要 `git push` 到 GitHub。
- 用户说“可以，继续开发”时，继续下一个小功能前，需要更新本文档，记录当前上下文。
- `Toide.pro` 是 Qt Creator/qmake 的手动编译入口，可以随代码变化同步维护。
- 后续允许使用 C++20 内容，但必须同步维护 `Toide.pro`，让 qmake/Qt Creator 明确传入 C++20 编译参数。
- 每次功能完成后优先运行：
  - CMake/CTest 测试。
  - qmake 构建。
  - 相关文件 lints。

## 当前技术栈

- 客户端：Qt 6.7 + C++20/Qt Widgets。
- 构建：
  - CMake：主自动化构建和测试。
  - qmake：`Toide.pro`，供 Qt Creator 手动打开编译；MinGW 显式使用 `-std=gnu++20`，MSVC 显式使用 `/std:c++20`。
- 仓库：GitHub `XINGMENGJ/Toide`。

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

## 最近一次问题

Qt Creator 页面编译失败，错误集中在：

- `std::optional` 不可用。
- `QProcess::startCommand()` 不存在。
- C++20 designated initializer 报警或失败。

处理方式：

- `TaskConfig::loadFromFile` 改为返回项目内的 `TaskConfigLoadResult`。
- 移除生产代码里的 C++20 designated initializer。
- `QProcess::startCommand()` 改为 Windows 使用 `cmd.exe /C`，非 Windows 使用 `/bin/sh -c`。

## 当前未提交改动

- `TaskRunnerWidget` 起步版。
- Qt Creator 兼容性修复。
- `Toide.pro` 显式配置 C++20 编译参数。
- 本文档。

## 下一步建议

下一次继续开发时，优先完成任务运行面板的“点击 Run 执行当前任务并显示输出”，然后提交并推送。
