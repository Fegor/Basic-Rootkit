# Hook Contracts: Basic Rootkit DLL

## Overview
The existing DLL installed under `Source/Basic Rootkit/` hides any process or file whose name contains the prefix `$pwn`. It achieves this by replacing two entries in the Import Address Table (IAT) of every target process.

## Contract 1: NtQuerySystemInformation
- **API**: `NTSTATUS NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG)`
- **Patch Site**: `ntdll.dll!NtQuerySystemInformation` (IAT thunk) inside the target process; replaced with `hkNtQuerySystemInformation`.
- **Purpose**: When the caller asks for `SystemProcessInformation`, remove any entry whose `ImageName` contains `$pwn`.
- **Hidden Object Rules**:
  - Works on both x86/x64; relies on `SYSTEM_PROCESS_INFORMATION.NextEntryOffset` manipulation.
  - Query must have succeeded (`STATUS_SUCCESS`) before tampering.
- **Rollback Trigger**:
  - Kill switch invoked from MFC harness.
  - DLL `DLL_PROCESS_DETACH` (e.g., process exit or manual unload).
  - On rollback we call `IAT::Hook("ntdll.dll", "NtQuerySystemInformation", lpOrgNtQuerySystemInformation)`.

## Contract 2: NtQueryDirectoryFile
- **API**: `NTSTATUS NtQueryDirectoryFile(...)`
- **Patch Site**: `ntdll.dll!NtQueryDirectoryFile` (IAT thunk) -> `hkNtQueryDirectoryFile`.
- **Purpose**: Strip `$pwn` entries when Explorer or Process Hacker enumerates directories (FileIdBothDirectoryInformation class).
- **Hidden Object Rules**:
  - Only touches results when `FileInformationClass == FileIdBothDirectoryInformation` and call succeeded.
  - Works by stitching the linked list of directory entries after removing matches.
- **Rollback Trigger**: Same as above, using stored `lpOrgNtQueryDirectoryFile` pointer.

## Safety Expectations
- Hooks must be installed via `CreateRemoteThread`+`InitHook`. No other API interception is permitted without updating this document.
- Kill switch must always call the rollback routine before offering the operator the option to reboot/exit.
- Any change to match logic (e.g., different prefixes) or additional hooks constitutes a MINOR constitution update and requires this contract to be amended.
