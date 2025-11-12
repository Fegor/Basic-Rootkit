#pragma once

#include <atlstr.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include "ProcessUtils.h"

struct InjectionSession
{
    std::wstring id;
    DWORD pid = 0;
    CString architecture;
    CString dllPath;
    ULONG_PTR remoteModule = 0;
    ULONGLONG startTick = 0;
};

struct InjectionResult
{
    bool success = false;
    CString error;
    InjectionSession session;
};

struct KillSwitchResult
{
    bool success = false;
    CString error;
    DWORD elapsedMs = 0;
};

class InjectionManager
{
public:
    InjectionManager();
    ~InjectionManager();

    bool SelectDll(const CString& path, CString& error);
    CString GetSelectedDll() const;
    bool HasSelectedDll() const;

    std::vector<ProcessInfo> ListProcesses() const;

    InjectionResult InjectIntoProcess(DWORD pid, const CString& targetArch);
    KillSwitchResult ActivateKillSwitch(const std::wstring& sessionId);

private:
    CString m_selectedDllPath;
    CString m_localArch;

    std::unordered_map<std::wstring, InjectionSession> m_sessions;
    mutable std::mutex m_mutex;

    InjectionResult PerformInjection(DWORD pid, const CString& targetArch);
    bool ValidateArchitecture(const CString& targetArch, CString& error) const;
};
