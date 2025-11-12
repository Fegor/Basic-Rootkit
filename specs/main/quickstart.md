# Quickstart: MFC DLL 测试程序

## Prerequisites
1. Windows 11 Pro lab host with Visual Studio 2022.3 (Desktop development with C++ workload + MFC)。
2. Optional Windows 7 VM（用于 x86 客户端验证）。
3. PowerShell 7 及 Pester 模块（Install-Module Pester -Scope CurrentUser）。

## Build Steps
1. 运行 pwsh ./lab-scripts/build/repro.ps1 -Configuration Release -Platform x86，脚本会同时编译 Rootkit DLL 与 Source/MfcHarness 可执行文件，并将哈希写入 lab-output/build-register.json。
2. 确认 lab-output/ 下的 uild-register.json 含有当前 git commit、语义版本与 SHA256。

## Test Steps
1. 合同测试：pwsh ./lab-scripts/tests/contract-injection.ps1 -DllPath <path>，验证 DLL 导出函数和控制端点可调用。
2. 回归测试：pwsh ./lab-scripts/tests/regression-hide.ps1 -ProcessName notepad.exe -FilePrefix ，确保隐藏效果。
3. 检测测试：pwsh ./lab-scripts/tests/detection-killswitch.ps1 -SessionId <id>，确保 Kill switch 在 5 秒内恢复可见性。

## Run the Harness
1. 启动 Source/MfcHarness/bin/Release/MfcHarness.exe（必须在离线实验室）。
2. "浏览 DLL" 选择 uild-register.json 中列出的 Rootkit DLL。
3. 刷新进程列表，选择与 DLL 架构相同的进程。
4. 点击“注入”，观察事件面板的 Hidden/Failure 状态。
5. 需要恢复时，点击“Kill Switch/反注入”，确认任务管理器和资源管理器恢复可见。
6. 如需导出事件，手动点击“导出事件”按钮，将 JSONL 写入 ./lab-output/telemetry.jsonl（会在会话结束时可安全删除）。

## Cleanup
- 执行脚本 pwsh ./lab-scripts/tests/detection-killswitch.ps1 -KillAll 确保所有 hook 已回滚。
- 重启 Windows 11 主机或回滚 VM 快照以销毁残留。
