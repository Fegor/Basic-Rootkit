# Implementation Plan: MFC DLL 测试程序

**Branch**: main | **Date**: 2025-11-10 | **Spec**: specs/main/spec.md
**Input**: Feature specification from /specs/main/spec.md

**Note**: This template is filled in by the /speckit.plan command. See .specify/templates/commands/plan.md for the execution workflow.

## Summary

Build a VC++ MFC desktop harness that loads the existing Basic Rootkit DLL, lets engineers pick a target process, inject the DLL, verify that $pwn-prefixed processes/files disappear, and expose a kill switch to restore visibility in under 5 seconds inside the isolated lab. The harness must surface telemetry in-app, export optional JSON lines, and drive deterministic builds/tests so the DLL + test app remain auditable.

## Technical Context

**Language/Version**: C++17 with Visual Studio 2022.3 (v143) toolset / MFC  
**Primary Dependencies**: MFC runtime, Win32 API (CreateToolhelp32Snapshot, OpenProcess, etc.), existing Rootkit DLL + IAT::Hook helper, CMake (optional for repro builds)  
**Storage**: N/A (in-memory state only; optional JSONL export to ./lab-output/)  
**Testing**: Lab integration tests scripted via PowerShell + manual validation (Process Hacker, Explorer), future xUnit-style regression harness (NEEDS CLARIFICATION: preferred framework)  
**Target Platform**: Windows 11 Pro host + Windows 7 VM (x86/x64) in isolated lab  
**Project Type**: single desktop project (MFC harness + existing Source/Basic Rootkit)  
**Performance Goals**: DLL injection + hide handshake <= 2 s; Kill switch restores visibility <= 5 s; UI remains responsive (<100 ms input lag)  
**Constraints**: Lab-only execution, no outbound network, deterministic build logs, kill switch mandatory for every session  
**Scale/Scope**: Single-operator research harness; supports dozens of lab sessions per week, <5 concurrent processes per test

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

1. **Controlled Research Deployment** - Covered by specs/main/spec.md "Operational Safeguards > Lab Boundary": work only on offline Win11 host + Win7 VM, destroy artifacts via reboot/snapshot, Jeff signs lab log. Harness will include banner reminding users of lab-only usage.
2. **Explicit Hook Contracts** - Rootkit DLL hooks NtQuerySystemInformation & NtQueryDirectoryFile via IAT patching (see Source/Basic Rootkit/dllmain.cpp). Plan deliverable: /docs/hook-contracts.md summarizing patch steps, hidden object filters, rollback conditions (kill switch + app exit). Harness simply triggers the DLL entry points and records outcomes.
3. **Deterministic Build & Traceability** - Implement scripts/build/repro.ps1 to invoke MSBuild with fixed flags, capture dependency hashes, embed semver+commit resource, and log outputs to lab-output/build-register.json. No unsigned artifacts allowed outside lab.
4. **Layered Observability & Safe Telemetry** - UI event panel keeps structured timeline; optional "导出日志" writes JSONL to local lab folder only. Telemetry never leaves host. Kill switch button doubles as telemetry boundary (logs success/failure).
5. **Test-First Hardening** - Before coding UI features, author: (a) contract test script ensuring DLL exports are callable, (b) regression test verifying $pwn objects hidden post-injection, (c) detection test ensuring kill switch restores baseline. Tests live in lab-scripts/tests/ and must run (and fail) before implementation.

All gates satisfied with concrete actions above; no blockers remain for Project Structure.

## Project Structure

### Documentation (this feature)

`	ext
specs/main/
├── plan.md              # This file
├── spec.md              # Completed feature spec
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/
│   └── injection.openapi.yaml  # Phase 1 API/contract descriptions
└── tasks.md             # Phase 2 output (/speckit.tasks)
`

### Source Code (repository root)

`	ext
Source/
├── Basic Rootkit/                # existing DLL source
└── MfcHarness/                   # new MFC app project
    ├── MfcHarness.sln
    ├── App/
    │   ├── MfcHarnessApp.cpp
    │   ├── MainDlg.cpp
    │   └── Resource.h
    ├── ui/
    │   └── telemetry-panel.xaml  # optional resource definitions
    └── build/
        └── repro.props           # shared deterministic flags

lab-scripts/
├── build/repro.ps1               # deterministic build driver
└── tests/
    ├── contract-injection.ps1
    ├── regression-hide.ps1
    └── detection-killswitch.ps1
`

**Structure Decision**: Single desktop project added under Source/MfcHarness plus supporting lab scripts; all documentation lives under specs/main. No multi-project decomposition required.

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| None | N/A | N/A |
