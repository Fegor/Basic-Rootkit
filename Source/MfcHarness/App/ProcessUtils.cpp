#include "pch.h"
#include <TlHelp32.h>
#include "ProcessUtils.h"

CString GetLocalProcessArchitecture()
{
#ifdef _WIN64
    return L"x64";
#else
    return L"x86";
#endif
}

bool HasInjectionPrivileges(DWORD pid)
{
    HANDLE processHandle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, pid);
    if (!processHandle)
    {
        return false;
    }
    ::CloseHandle(processHandle);
    return true;
}

CString DescribeProcessArchitecture(DWORD pid)
{
    HANDLE processHandle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!processHandle)
    {
        return L"?";
    }

    BOOL isWow64 = FALSE;
    CString arch = GetLocalProcessArchitecture();
    using FnIsWow64 = BOOL (WINAPI *)(HANDLE, PBOOL);
    auto fnIsWow64 = reinterpret_cast<FnIsWow64>(::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "IsWow64Process"));

#ifdef _WIN64
    arch = L"x64";
#else
    arch = L"x86";
#endif

    if (fnIsWow64 && fnIsWow64(processHandle, &isWow64))
    {
#ifdef _WIN64
        arch = isWow64 ? L"x86" : L"x64";
#else
        arch = isWow64 ? L"x86" : L"?";
#endif
    }

    ::CloseHandle(processHandle);
    return arch;
}

std::vector<ProcessInfo> EnumerateProcesses()
{
    std::vector<ProcessInfo> results;
    HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return results;
    }

    PROCESSENTRY32W entry{ sizeof(PROCESSENTRY32W) };
    if (!::Process32FirstW(snapshot, &entry))
    {
        ::CloseHandle(snapshot);
        return results;
    }

    const DWORD currentPid = ::GetCurrentProcessId();
    do
    {
        ProcessInfo info;
        info.pid = entry.th32ProcessID;
        info.name = entry.szExeFile;
        info.architecture = DescribeProcessArchitecture(entry.th32ProcessID);
        info.injectable = (entry.th32ProcessID != currentPid) && HasInjectionPrivileges(entry.th32ProcessID);
        results.emplace_back(info);
    } while (::Process32NextW(snapshot, &entry));

    ::CloseHandle(snapshot);
    return results;
}
