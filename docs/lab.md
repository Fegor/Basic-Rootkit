# Lab Environment: Basic Rootkit Research Harness

## Boundary & Topology
- **Host**: Windows 11 Pro 23H2 workstation (offline). All development, builds, and manual validation happen here.
- **Guest VM**: Windows 7 SP1 (x86) inside Hyper-V (can be swapped for VMware). VM network mode is NAT-only with no route to production networks.
- **Network Rules**: Both host and VM stay disconnected from corporate/personal networks. Only loopback (localhost) is permitted; outbound internet is disabled via firewall profile "Lab-Isolated".
- **Tooling Whitelist**: Visual Studio 2022.3, PowerShell 7 + Pester, Process Hacker, Explorer, Task Manager, custom lab scripts under `lab-scripts/`. Any new tool requires maintainer (Jeff) approval.

## Data Handling & Disposal
- Use synthetic `$pwn`-prefixed processes/files only. Do not ingest real customer data.
- Capture telemetry only in memory; optional exports go to `./lab-output/`. Destroy files at session end using `lab-output/cleanup.ps1` (to be implemented) or VM snapshot revert.
- After each experiment, reboot host + revert VM snapshot to guarantee hook removal and delete crash dumps/logs.

## Compliance Workflow
1. Before work: Jeff signs the lab log noting experiment scope, DLL build hash, and VM snapshot ID.
2. During work: maintain experiment notes (process names, timestamps) in `docs/usage/mfc-harness.md` appendix.
3. After work: verify kill switch success, run cleanup script, reboot/revert, and sign completion entry.

## Emergency Procedures
- If hidden processes/files fail to reappear within 5 seconds after kill switch, immediately trigger manual system reboot.
- If DLL injection causes instability, power off VM via Hyper-V manager, then run host kill switch + reboot.
- Console warning banner (to be added in MFC app) reminds operators this setup is lab-only.
