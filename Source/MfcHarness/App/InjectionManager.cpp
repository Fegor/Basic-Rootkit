#include "pch.h"
#include "InjectionManager.h"
#include <memory>

namespace
{
CString MakeGuidString()
{
    GUID guid;
    if (FAILED(::CoCreateGuid(&guid)))
    {
        return {};
    }
    wchar_t buffer[64] = {0};
    ::StringFromGUID2(guid, buffer, 64);
    return buffer;
}

CString FormatErrorMessage(DWORD errorCode)
{
    LPWSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = ::FormatMessageW(flags, nullptr, errorCode, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    CString result = (length > 0 && buffer) ? buffer : L"操作失败";
    if (buffer)
    {
        ::LocalFree(buffer);
    }
    return result;
}
}

InjectionManager::InjectionManager()
    : m_localArch(GetLocalProcessArchitecture())
{
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
}

InjectionManager::~InjectionManager()
{
    ::CoUninitialize();
}

bool InjectionManager::SelectDll(const CString& path, CString& error)
{
    DWORD attributes = ::GetFileAttributes(path);
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        error = L"DLL 路径不存在或无法访问";
        return false;
    }
    m_selectedDllPath = path;
    return true;
}

CString InjectionManager::GetSelectedDll() const
{
    return m_selectedDllPath;
}

bool InjectionManager::HasSelectedDll() const
{
    return !m_selectedDllPath.IsEmpty();
}

std::vector<ProcessInfo> InjectionManager::ListProcesses() const
{
    return EnumerateProcesses();
}

bool InjectionManager::ValidateArchitecture(const CString& targetArch, CString& error) const
{
    if (targetArch.CompareNoCase(m_localArch) != 0)
    {
        error.Format(L"架构不匹配：目标为 %s，Harness 为 %s", targetArch.GetString(), m_localArch.GetString());
        return false;
    }
    return true;
}

InjectionResult InjectionManager::InjectIntoProcess(DWORD pid, const CString& targetArch)
{
    return PerformInjection(pid, targetArch);
}

InjectionResult InjectionManager::PerformInjection(DWORD pid, const CString& targetArch)
{
    InjectionResult result;

    if (!HasSelectedDll())
    {
        result.error = L"请先选择 DLL";
        return result;
    }

    CString archError;
    if (!ValidateArchitecture(targetArch, archError))
    {
        result.error = archError;
        return result;
    }

    HANDLE processHandle = ::OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
    if (!processHandle)
    {
        result.error = FormatErrorMessage(::GetLastError());
        return result;
    }

    const size_t bytes = (m_selectedDllPath.GetLength() + 1) * sizeof(wchar_t);
    LPVOID remoteBuffer = ::VirtualAllocEx(processHandle, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuffer)
    {
        result.error = L"无法在目标进程中分配内存";
        ::CloseHandle(processHandle);
        return result;
    }

    if (!::WriteProcessMemory(processHandle, remoteBuffer, m_selectedDllPath.GetString(), bytes, nullptr))
    {
        result.error = L"写入 DLL 路径失败";
        ::VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
        ::CloseHandle(processHandle);
        return result;
    }

    const auto loadLibraryW = reinterpret_cast<LPTHREAD_START_ROUTINE>(::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "LoadLibraryW"));
    if (!loadLibraryW)
    {
        result.error = L"无法定位 LoadLibraryW";
        ::VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
        ::CloseHandle(processHandle);
        return result;
    }

    HANDLE threadHandle = ::CreateRemoteThread(processHandle, nullptr, 0, loadLibraryW, remoteBuffer, 0, nullptr);
    if (!threadHandle)
    {
        result.error = FormatErrorMessage(::GetLastError());
        ::VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
        ::CloseHandle(processHandle);
        return result;
    }

    const DWORD waitCode = ::WaitForSingleObject(threadHandle, 15000);
    if (waitCode != WAIT_OBJECT_0)
    {
        result.error = L"注入线程超时或失败";
        ::CloseHandle(threadHandle);
        ::VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
        ::CloseHandle(processHandle);
        return result;
    }

    DWORD remoteModule = 0;
    ::GetExitCodeThread(threadHandle, &remoteModule);

    ::CloseHandle(threadHandle);
    ::VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
    ::CloseHandle(processHandle);

    if (remoteModule == 0)
    {
        result.error = L"目标进程未返回有效模块句柄";
        return result;
    }

    InjectionSession session;
    session.id = std::wstring(MakeGuidString().GetString());
    session.pid = pid;
    session.architecture = targetArch;
    session.dllPath = m_selectedDllPath;
    session.remoteModule = static_cast<ULONG_PTR>(remoteModule);
    session.startTick = ::GetTickCount64();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions[session.id] = session;
    }

    result.success = true;
    result.session = session;
    return result;
}

KillSwitchResult InjectionManager::ActivateKillSwitch(const std::wstring& sessionId)
{
    KillSwitchResult result;

    InjectionSession session;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(sessionId);
        if (it == m_sessions.end())
        {
            result.error = L"找不到指定的注入会话";
            return result;
        }
        session = it->second;
        m_sessions.erase(it);
    }

    HANDLE processHandle = ::OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ, FALSE, session.pid);
    if (!processHandle)
    {
        result.error = FormatErrorMessage(::GetLastError());
        return result;
    }

    const auto freeLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "FreeLibrary"));
    if (!freeLibrary)
    {
        result.error = L"无法定位 FreeLibrary";
        ::CloseHandle(processHandle);
        return result;
    }

    HANDLE threadHandle = ::CreateRemoteThread(processHandle, nullptr, 0, freeLibrary, reinterpret_cast<LPVOID>(session.remoteModule), 0, nullptr);
    if (!threadHandle)
    {
        result.error = FormatErrorMessage(::GetLastError());
        ::CloseHandle(processHandle);
        return result;
    }

    const ULONGLONG start = ::GetTickCount64();
    const DWORD waitCode = ::WaitForSingleObject(threadHandle, 5000);
    result.elapsedMs = static_cast<DWORD>(::GetTickCount64() - start);

    if (waitCode != WAIT_OBJECT_0)
    {
        result.error = L"Kill switch 超时或失败";
        ::CloseHandle(threadHandle);
        ::CloseHandle(processHandle);
        return result;
    }

    DWORD exitCode = 0;
    ::GetExitCodeThread(threadHandle, &exitCode);

    ::CloseHandle(threadHandle);
    ::CloseHandle(processHandle);

    if (exitCode == 0)
    {
        result.error = L"FreeLibrary 返回失败";
        return result;
    }

    result.success = true;
    return result;
}
