# AGENTS.md

基于 raylib 的 C++17 桌面游戏程序（技术验证阶段），`core/` 为单一可执行目标。架构细节见 `docs/项目架构文档.md`，需求/漏洞文档索引见 `docs/README.md`。

## 工作流规范（必须遵守）

### 需求开发
1. 先在 `docs/demand/` 创建 `YYYMMDD-序号-需求名称.md`，包含「需求背景 / 需求目标 / 设计方案」。
2. 编写需求文档后必须询问用户修改意见并确认，确认后才可开始实现。
3. 完成后在 `docs/README.md` 登记一行需求索引。

### 漏洞修复
1. 仅在用户明确提出漏洞修复需求时才记录；实现需求过程中自行发现并修复的问题不记录。
2. 先在 `docs/bug/` 创建 `YYYMMDD-序号-漏洞名称.md`，包含「漏洞现象 / 漏洞原因 / 修复方案」。
3. 修复方案需用户确认后执行；完成后在 `docs/README.md` 登记漏洞索引。

### 架构变更
- 更新依赖库、架构或关键设计时，同步更新 `docs/项目架构文档.md`。

## 代码格式

- 所有生成或修改的 C++ 代码必须严格遵循根目录 `.clang-format` 的格式（BasedOnStyle: Microsoft，缩进 4 空格、禁用 Tab、行宽 120、`PointerAlignment: Left`、include 自动排序并 Regroup）。提交前用 clang-format 格式化，确保与配置文件一致。

## 构建与运行

- 只通过 CMake preset 构建：Windows 用 `msvc-debug` / `msvc-release`，Linux 用 `linux-debug` / `linux-release`。
- 命令（需在 "x64 Native Tools Command Prompt for VS" 中，或先运行 `vcvars64.bat`）：
  ```
  cmake --preset msvc-debug
  cmake --build --preset msvc-debug
  ```

## 验证

游戏项目，所有修改与验证操作需用户手动进行：完成修改后启动程序，提示用户验证，用户确认通过后才可继续下一步。
