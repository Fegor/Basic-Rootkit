# Data Model: MFC DLL 测试程序

## Entity: TargetProcess
- **Purpose**: Represent a running process that can host the Rootkit DLL.
- **Fields**:
  - processId (DWORD) – system PID.
  - 
ame (wstring) – executable name.
  - ullPath (wstring) – absolute path for validation.
  - rchitecture (enum: x86/x64) – used to match DLL build.
  - sessionId (DWORD) – to filter interactive sessions.
  - isInjectable (bool) – derived flag after access-rights check.

## Entity: InjectionSession
- **Purpose**: Track each DLL injection attempt and verification outcome.
- **Fields**:
  - sessionId (GUID) – unique per attempt.
  - dllPath (wstring) – path selected by user.
  - 	argetProcessId (DWORD) – FK -> TargetProcess.processId (snapshot copy).
  - 	argetProcessArch (enum) – to ensure correct DLL flavor.
  - startTimeUtc (FILETIME) / endTimeUtc – timing for SLA checks.
  - status (enum: Pending, Injecting, HiddenVerified, Failed, RolledBack).
  - errorMessage (wstring, optional).
  - hiddenObjects (struct) – counts of processes/files hidden.

## Entity: TelemetryEvent
- **Purpose**: Provide in-app audit trail for observability.
- **Fields**:
  - 	imestampUtc (FILETIME).
  - level (enum: Info, Warning, Error).
  - eventType (enum: SelectDll, SelectProcess, InjectStart, InjectResult, KillSwitch, ExportLog).
  - payload (JSON string) – structured data for export.
  - sessionId (GUID, optional) – links to InjectionSession.

## Relationships
- InjectionSession references a snapshot of TargetProcess; we store the metadata at injection time to keep history even if process exits.
- TelemetryEvent.sessionId (optional) associates events with a session while allowing UI-level events (e.g., export) without an active injection.

## Validation Rules
- dllPath MUST point to a file signed/hashed in the current lab build register.
- 	argetProcessArch MUST equal DLL architecture; mismatch blocks injection.
- hiddenObjects.processCount / ileCount MUST be >=0 and recorded before and after kill switch to compute restoration success.
- status transitions: Pending → Injecting → (HiddenVerified | Failed); HiddenVerified → RolledBack only via kill switch.
