#pragma once

#include <string>
#include <atlstr.h>

inline std::string WideToUtf8(const std::wstring& value)
{
    if (value.empty()) { return std::string(); }
    const int sizeNeeded = ::WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(sizeNeeded, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), sizeNeeded, nullptr, nullptr);
    return result;
}

inline std::wstring Utf8ToWide(const std::string& value)
{
    if (value.empty()) { return std::wstring(); }
    const int sizeNeeded = ::MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(sizeNeeded, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), sizeNeeded);
    return result;
}

inline std::string CStringToUtf8(const CString& value)
{
    return WideToUtf8(std::wstring(value));
}

inline CString Utf8ToCString(const std::string& value)
{
    std::wstring wide = Utf8ToWide(value);
    return CString(wide.c_str());
}
