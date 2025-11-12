#pragma once

#include <vector>
#include <atlstr.h>

struct ProcessInfo
{
    DWORD pid = 0;
    CString name;
    CString architecture;
    bool injectable = false;
};

CString GetLocalProcessArchitecture();
std::vector<ProcessInfo> EnumerateProcesses();
CString DescribeProcessArchitecture(DWORD pid);
bool HasInjectionPrivileges(DWORD pid);
