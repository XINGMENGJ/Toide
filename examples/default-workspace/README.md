# Toide 示例工作区

这是 Toide 启动时默认打开的示例工作区，用来快速展示客户端能力，并适合测试 **底栏「编译」**（对 `include/` 与 `src/` 做 `g++ -fsyntax-only`）。

## 目录说明

- `include/example/msg.h` — 示例头文件
- `src/main.cpp` — 程序入口
- `src/greeting.cpp` — 与头文件配套的实现
- `diagnostics/diagnostic_demo.cpp` — **故意写错语法**，仅给任务「Build Diagnostics Demo」用，**不在** `src/` 下，避免与「编译」页一次性检查所有 `src/*.cpp` 冲突

## 你可以体验什么

- 左侧文件树：浏览目录和文件。
- 中间编辑器：打开、编辑、保存（`Ctrl+S`）。
- 底栏 **「编译」**：应能通过语法检查（1 个头文件 + 2 个 `.cpp`）；**运行（链接并执行）** 可生成 `.toide-build\toide-workspace-run.exe` 并显示控制台输出（主工具栏亦有 **运行工作区**）。
- 底栏 **Tasks**：`Build Example` 会用 `g++` 链接生成 `build\hello_toide.exe`；`Run Example` 运行它。
- **Build Diagnostics Demo**：编译 `diagnostics\diagnostic_demo.cpp`，用于看报错与诊断跳转。

## 建议操作

1. 打开 `src/main.cpp`，可选改一改输出逻辑（在 `greeting.cpp` 的 `build_greeting` 里）。
2. 保存后，在 **「编译」** 页点击 **编译 include 与 src**，确认输出无错误。
3. 在 Tasks 里运行 **Build Example**，再 **Run Example**。
4. 运行 **Build Diagnostics Demo** 查看故意触发的编译错误。

`Run Example` 会加载仓库中的 `qt6.7-env.cmd`（Qt 6.11 MinGW），避免找不到 `g++` 或运行期 DLL。若提示缺少 `build\hello_toide.exe`，请先成功执行 **Build Example**。
