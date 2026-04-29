# Toide 示例工作区

这是 Toide 启动时默认打开的示例工作区，用来快速展示当前客户端已经具备的能力。

## 你可以体验什么

- 左侧文件树：浏览这个示例工作区的目录和文件。
- 中间编辑器：双击 Markdown、C++、JSON 文件后打开标签页。
- 保存文件：修改文件后使用 `Ctrl+S` 或工具栏 Save。
- 底部 Tasks 面板：读取 `.toide/tasks.json` 并显示可运行任务。
- 任务输出：点击 Run 后可在输出区域看到命令输出。
- 示例编译：选择 `Build Example` 会先加载仓库里的 Qt 6.7 MinGW 环境，再用 `g++` 编译 `src/hello_toide.cpp`。
- 诊断验证：选择 `Build Diagnostics Demo` 会故意编译失败，用来观察编译错误输出。

## 建议操作

1. 双击打开 `src/hello_toide.cpp`。
2. 修改文件中的输出文本。
3. 按 `Ctrl+S` 保存。
4. 在底部 Tasks 面板选择 `Build Example`。
5. 点击 Run，等待输出区显示编译完成。
6. 选择 `Run Example` 并点击 Run，查看程序输出。
7. 选择 `Build Diagnostics Demo` 并点击 Run，查看 `src/diagnostic_demo.cpp` 的错误输出。

`Run Example` 也会加载同一个 Qt 6.7 MinGW 环境，避免运行时找不到 MinGW 运行库 DLL。如果它提示缺少 `build\\hello_toide.exe`，说明编译没有成功完成，请先查看 `Build Example` 的输出。

## 后续会加入

- Git 状态面板。
- 项目成员列表。
- 文件版本和冲突检测。
- WebSocket 协作事件。
