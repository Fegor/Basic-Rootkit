# Research: MFC DLL Harness

## Item 1: Test framework & execution
- **Decision**: Use PowerShell + Pester scripts in `lab-scripts/tests/` to drive contract, regression, detection suites. Pester can optionally shell out to small helper binaries if needed.
- **Rationale**: Ships with Windows/Pwsh, has rich assertions, and can orchestrate Process Hacker / Explorer checks in the same language.
- **Alternatives considered**:
  1. GoogleTest – requires separate runner per architecture and complicates process manipulation.
  2. Pure manual testing – violates Test-First gate and is hard to audit.

## Item 2: Hook contract documentation
- **Decision**: Document the two IAT hooks (`NtQuerySystemInformation`, `NtQueryDirectoryFile`) in `docs/hook-contracts.md` with patch site, filtering rule, rollback entry point.
- **Rationale**: Keeps constitution compliance explicit while letting the harness reuse the existing DLL.
- **Alternatives considered**: rely on source comments (insufficient) or duplicate logic in harness (drifts quickly).

## Item 3: Target enumeration strategy
- **Decision**: Use ToolHelp snapshot APIs for processes and Win32 file enumeration for `$pwn` artifacts. Tag each process with architecture to match DLL build.
- **Rationale**: Works inside isolated lab without WMI/ETW dependencies, aligns with how the DLL filters results.
- **Alternatives considered**: WMI (often disabled) or ETW (overkill).

## Item 4: Deterministic build script
- **Decision**: `lab-scripts/build/repro.ps1` calls MSBuild with `/p:Deterministic=true /p:EmbedUntrackedSources=true`, hashes every DLL/EXE touched after the build, and records metadata (semver, git commit, toolchain) in `lab-output/build-register.json`.
- **Rationale**: Centralizes reproducibility evidence and works for both the legacy DLL and the new harness.
- **Alternatives considered**: migrate to CMake now (high cost) or rely on ad-hoc builds.

## Item 5: Telemetry & retention
- **Decision**: Maintain an in-memory ring buffer of the last 100 events for UI debugging. Only when the operator clicks "导出事件" do we flush JSONL records (schema: `{timestamp, level, eventType, payload}`) to `./lab-output/telemetry.jsonl`. Files are deleted during `lab-output/cleanup.ps1` or by VM snapshot revert; retention window is effectively "current session only" (≤1 day).
- **Rationale**: Satisfies observability requirements without persisting logs by default, matching the "no logs" preference.
- **Alternatives considered**: always-on logging (too risky) or no telemetry at all (hurts diagnosability).
