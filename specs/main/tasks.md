---

description: "Task list for MFC DLL 测试程序"
---

# Tasks: MFC DLL 测试程序

**Input**: Design documents from /specs/main/
**Prerequisites**: plan.md (ready), spec.md, research.md, data-model.md, contracts/

**Tests**: Constitution mandates contract/regression/detection suites; mark all as blocking until executed.

**Organization**: Tasks grouped by constitution gates first, then user story (P1) to preserve independent delivery.

## Format: [ID] [P?] [Story] Description

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = DLL 隐藏进程与文件
- Include exact file paths in descriptions

## Constitution Alignment (Pre-Phase)

**Purpose**: Complete before Phase 1 to satisfy governance.

- [ ] T000 docs/lab.md — 记录 Windows 11 主机 + Win7 VM 的隔离拓扑、快照/销毁流程与合规签名人。
- [ ] T000a docs/hook-contracts.md — 描述 NtQuerySystemInformation / NtQueryDirectoryFile IAT hook 的补丁点、隐藏规则、回滚触发条件。
- [ ] T000b lab-scripts/build/repro.ps1 — 编写确定性构建脚本，输出 lab-output/build-register.json（含 SHA256、git commit、semver、工具链）。
- [ ] T000c specs/main/research.md — 追加 telemetry 导出策略细节（JSONL 结构、保留 30 天 -> NEEDS decision）并获得认可。

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: 建立项目骨架与共用脚本。

- [ ] T001 Source/MfcHarness/ – 创建 MFC App (对话框) 工程，配置 v143 工具集、/permissive-、/guard:cf、/DYNAMICBASE。
- [ ] T002 Source/MfcHarness/App/MainDlg.cpp — 加入 UI 元素：DLL 路径浏览、进程列表、事件面板、Kill switch 按钮。
- [ ] T003 [P] lab-output/ – 初始化目录并添加 .gitkeep，确保日志/构建注册表集中存放。
- [ ] T004 lab-scripts/tests/ – 搭建 Pester 测试框架的通用模块（Connect-Injector.psm1）。

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: 运行任何用户故事前需要完成。

- [ ] T005 Source/MfcHarness/App/ProcessEnumerator.cpp — 实现基于 ToolHelp32 的进程枚举，映射架构/SessionId；暴露 IsInjectable 判定。
- [ ] T006 Source/MfcHarness/App/DllValidator.cpp — 校验 DLL 路径是否存在于 lab-output/build-register.json 且架构匹配。
- [ ] T007 [P] lab-scripts/tests/contract-injection.ps1 — 编写合同测试，确保 DLL 导出可加载、Harness 控制端点 /dll、/processes 可调用；运行并记录失败截图。
- [ ] T008 lab-scripts/tests/regression-hide.ps1 — 模拟注入流程，确认 $pwn 前缀进程/文件在 Task Manager/Explorer 中不可见。
- [ ] T009 lab-scripts/tests/detection-killswitch.ps1 — 验证 Kill switch 完成后 5 秒内所有 hook 移除，出现异常时提示用户执行重启。
- [ ] T010 lab-scripts/tests/common.psm1 — 实现共享断言（例如 Wait-ForProcessHidden、Wait-ForProcessVisible）。

**Checkpoint**: 合同/回归/检测测试均能 FAIL-then-PASS；UI 仍可运行空白流程。

---

## Phase 3: User Story 1 - DLL 隐藏进程与文件 (Priority: P1) 🎯 MVP

**Goal**: 让研究人员通过 MFC 界面加载 DLL、选择进程、注入并验证隐藏；提供 Kill switch。

**Independent Test**: 运行三大 Pester 脚本 + 手工确认任务管理器/资源管理器结果。

### Tests for User Story 1 (BLOCKING)

- [ ] T011 [P][US1] lab-scripts/tests/contract-injection.ps1 — 对接真实 UI，确保按钮事件触发脚本 API。
- [ ] T012 [P][US1] lab-scripts/tests/regression-hide.ps1 — 在 Win11 主机 + Win7 VM 分别执行，记录隐藏截图。
- [ ] T013 [P][US1] lab-scripts/tests/detection-killswitch.ps1 — 验证 Kill switch SLA；失败时产生 incident 报告。

### Implementation for User Story 1

- [ ] T014 [US1] Source/MfcHarness/App/MainDlg.cpp — 绑定“浏览 DLL”按钮到 DllValidator，展示哈希/版本信息。
- [ ] T015 [US1] Source/MfcHarness/App/MainDlg.cpp — 进程列表支持刷新、过滤架构、禁用非可注入条目。
- [ ] T016 [US1] Source/MfcHarness/App/Injector.cpp — 调用 CreateRemoteThread/LoadLibraryW 注入 DLL，跟踪 InjectionSession 状态。
- [ ] T017 [US1] Source/MfcHarness/App/TelemetryPanel.cpp — 渲染事件时间线，并在用户选择时导出到 lab-output/telemetry.jsonl。
- [ ] T018 [US1] Source/MfcHarness/App/KillSwitchController.cpp — 调用 DLL 提供的卸载入口或重新注入干净 stub，确保 5 秒内恢复；失败时提示用户执行重启。
- [ ] T019 [US1] docs/usage/mfc-harness.md — 记录 UI 操作、lab-only 警告、Kill switch 步骤与验证方法。

**Checkpoint**: UI、脚本、Kill switch 与文档齐备，可在实验室内独立演示。

---

## Phase N: Polish & Cross-Cutting Concerns

- [ ] T020 docs/hook-contracts.md — 添加图示或表格，说明 IAT patch offsets 及 rollback 流程（与 DLL 代码保持同步）。
- [ ] T021 [P] Source/MfcHarness/App/ErrorHandling.cpp — 统一弹窗/日志格式，确保 Edge case “目标不存在” 提示包含三种操作选项。
- [ ] T022 lab-output/cleanup.ps1 — 自动清除 telemetry/export、重置 build register 并触发 VM 快照回滚。
- [ ] T023 README.md — 新增“Lab harness” 章节，引导读者至 docs/usage/mfc-harness.md。

## Dependencies & Execution Order

- Constitution Alignment 完成之前不得开始 Phase 1。
- Phase 2 需要 Phase 1 产出的项目骨架与脚本目录。
- Pester 测试 (T011–T013) 必须 FAIL → PASS 顺序；实现任务 (T014–T018) 不得在测试 stub 完成前合入。
- Polish 任务（T020+）可在 MVP 完成后插入，但 Hook 合同文档应与实现同步更新。
