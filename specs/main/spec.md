# Feature Specification: MFC DLL 测试程序

**Feature Branch**: main  
**Created**: 2025-11-10  
**Status**: Draft  
**Input**: User description: "需要 VC++ MFC 写一个测试程序"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - DLL 隐藏进程与文件 (Priority: P1)

用户打开 MFC 程序，浏览并加载根项目 DLL，选择目标进程后点击“注入”。成功注入后，该 DLL 会在目标进程中隐藏指定前缀的进程与文件项。用户可通过任务管理器与资源管理器验证隐藏效果，并可触发 kill switch 恢复显示。

**Why this priority**: 验证 DLL 隐藏功能是整个项目的核心能力。

**Independent Test**: 在隔离实验室中准备虚拟进程与带标记前缀的虚拟文件，比较注入前后的可见性差异，以及 kill switch 的恢复结果。

**Acceptance Scenarios**:

1. **Given** 已在隔离实验室中启动受控目标进程和带 $pwn 前缀的文件，**When** 用户在 MFC 程序中选择 DLL 与目标进程并点击“注入”，**Then** 该进程在任务管理器中不可见、该文件在资源管理器中不可见，但实际运行未受影响。
2. **Given** DLL 已注入且目标对象被隐藏，**When** 用户点击 MFC 程序中的“Kill Switch / 反注入”，**Then** 所有目标进程与文件在 5 秒内重新可见且无残留 hook。

### Edge Cases

- 注入过程中发生异常时必须提示用户可选“退出程序”“重启系统”“继续”三种方式，并在日志/状态窗中记录原因（NEEDS CLARIFICATION: 记录介质与格式）。
- 用户选定的目标进程在注入前已经退出或受系统保护时，程序必须阻止继续操作并提供恢复指引；若用户选择“重启”则需在 kill switch 完成后提示安全断电流程。

## Operational Safeguards *(constitution-mandated)*

### Lab Boundary & Compliance
- 实验室范围限制在离线的 Windows 11 主机与本地 Hyper-V/VMware Windows 7 虚拟机，二者均隔离于生产网络并使用 NAT-only 访问。  
- 测试数据仅限合成的虚拟进程与测试文件，操作完成后通过“系统重启 + VM 快照回滚”方式销毁。  
- 合规审核人：Jeff（项目维护者）；执行前后需在实验记录中签名。

### Hook Contracts
- 本功能复用现有 Rootkit DLL 的 IAT Hook：NtQuerySystemInformation（隐藏进程条目）与 NtQueryDirectoryFile（隐藏文件/目录条目）。  
- MFC 程序必须在 UI 中引用这些 hook 合同，标明：注入时打补丁，Kill switch 调用 IAT::Hook 回滚到 lpOrg* 指针。  
- NEEDS CLARIFICATION: 现有 DLL hook 合同尚未有单独文档，需在 /docs/hook-contracts.md 中补充原子补丁、期望副作用与回滚触发条件。

### Observability & Kill Switch
- Telemetry 通过 MFC 界面的“事件面板”以结构化 JSON 行的方式存放在内存队列，并在用户点击“导出”时写入 ./lab-output/telemetry.jsonl（默认不落盘以符合当前“不要日志”偏好）。  
- 每次注入/反注入都会记录：时间戳、进程 ID、目标文件前缀、结果代码。  
- Kill switch 为 UI 按钮，触发后调用 DLL 的恢复入口并在 5 秒内验证可见性；若失败必须提示用户执行重启。

### Deterministic Build Notes
- 工具链：Visual Studio 2022.3 (v143) + CMakePresets.json 中的 x64/x86 Release/Debug 配置（NEEDS CLARIFICATION: CMake 是否实际使用）。  
- MFC 测试程序与 DLL 均需通过 scripts/build/repro.ps1（待创建）记录 cl.exe 标志、依赖哈希，并在生成的 EXE/DLL 中写入 semver + git commit 资源节。  
- 构建完成后产物与哈希写入 ./lab-output/build-register.json，保持 30 天后清除。

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: 程序必须允许用户浏览本地磁盘并加载指定的 Rootkit DLL。
- **FR-002**: 程序必须列出当前会话中可注入的目标进程（含 PID、路径、架构）。
- **FR-003**: 程序必须执行 DLL 注入，并在 UI 中实时展示注入状态与错误信息。
- **FR-004**: 程序必须提供 Kill switch / 反注入按钮，确保在 5 秒内恢复所有 hook。
- **FR-005**: 程序必须在 UI 事件面板中显示最近一次注入、隐藏验证与 Kill switch 结果；导出到文件为可选步骤。

### Key Entities *(include if feature involves data)*

- **InjectionSession**: 记录单次 DLL 注入的 DLL 路径、目标进程信息、开始/结束时间、结果状态以及可见性验证摘要。
- **TargetProcess**: 具有 PID、可执行路径、架构 (x86/x64)、当前可见性标志；提供是否适配本 DLL 的校验方法。

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 针对 $pwn 前缀的测试进程与文件，注入后隐藏成功率 100%，且在 2 次独立实验中复现。
- **SC-002**: Kill switch 在 5 秒内恢复可见性且回滚所有 hook，失败率低于 1%。

