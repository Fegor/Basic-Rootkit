#include "IAT Hook.h"

#include <TlHelp32.h>
#include <Psapi.h>
#include <strsafe.h>
#include <Shlwapi.h>

/**
 * Function who retrieve the base address of the main module of the current process.
 * \return : the base address if it's successfull else nullptr
 */
LPVOID IAT::GetCurrentProcessModule()
{
	// ✅ 正确声明：WCHAR 数组，不是指针数组
	WCHAR lpImageName[MAX_PATH];

	// ✅ 使用 Unicode 版本
	GetProcessImageFileNameW(GetCurrentProcess(), lpImageName, MAX_PATH);

	// ✅ 只定义一次，使用 W 版本
	MODULEENTRY32W ModuleList{};
	ModuleList.dwSize = sizeof(MODULEENTRY32W);

	const HANDLE hProcList = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	if (hProcList == INVALID_HANDLE_VALUE)
		return nullptr;

	// ✅ 使用 W 版本
	if (!Module32FirstW(hProcList, &ModuleList))
	{
		CloseHandle(hProcList);
		return nullptr;
	}

	// ✅ 第一个模块检查
	if (StrStrIW(ModuleList.szModule, lpImageName) != nullptr)
	{
		CloseHandle(hProcList);
		return ModuleList.hModule;
	}

	// ✅ 注意括号和参数顺序
	while (Module32NextW(hProcList, &ModuleList))
	{
		if (StrStrIW(ModuleList.szModule, lpImageName) != nullptr)
		{
			CloseHandle(hProcList);
			return ModuleList.hModule;
		}
	}

	CloseHandle(hProcList);
	return nullptr;
}
/**
 * Function to hook functions in the IAT of a specified module.
 * \param lpModuleName : name of the module wich contains the function you want to hook.
 * \param lpFunctionName : name of the function you want to hook.
 * \param lpFunction : pointer of the new function.
 * \param lpTargetModuleName : name of the module you want to target.
 * \return : the pointer of the original function or nullptr if it failed.
 */
LPVOID IAT::Hook(LPCSTR lpModuleName, LPCSTR lpFunctionName, const LPVOID lpFunction, LPCSTR lpTargetModuleName)
{
	const HANDLE hModule = GetModuleHandleA(lpTargetModuleName);
	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)(hModule);
	if (lpImageDOSHeader == nullptr)
		return nullptr;

	const auto lpImageNtHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);

	const IMAGE_DATA_DIRECTORY ImportDataDirectory = lpImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	auto lpImageImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD_PTR)hModule + ImportDataDirectory.VirtualAddress);

	while (lpImageImportDescriptor->Characteristics != 0)
	{
		const auto lpCurrentModuleName = (LPSTR)((DWORD_PTR)lpImageDOSHeader + lpImageImportDescriptor->Name);
		if (strcmp(lpCurrentModuleName, lpModuleName) != 0)
		{
			lpImageImportDescriptor++;
			continue;
		}

		auto lpImageOrgThunkData = (PIMAGE_THUNK_DATA)((DWORD_PTR)lpImageDOSHeader + lpImageImportDescriptor->OriginalFirstThunk);
		auto lpImageThunkData = (PIMAGE_THUNK_DATA)((DWORD_PTR)lpImageDOSHeader + lpImageImportDescriptor->FirstThunk);

		while (lpImageOrgThunkData->u1.AddressOfData != 0)
		{
			const auto lpImportData = (PIMAGE_IMPORT_BY_NAME)((DWORD_PTR)lpImageDOSHeader + lpImageOrgThunkData->u1.AddressOfData);

			if (strcmp(lpFunctionName, lpImportData->Name) == 0)
			{
				DWORD dwJunk = 0;
				MEMORY_BASIC_INFORMATION mbi;

				VirtualQuery(lpImageThunkData, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
				if (!VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &mbi.Protect))
					return nullptr;

				const auto lpOrgFunction = (LPVOID)lpImageThunkData->u1.Function;

				#if defined _M_IX86
					lpImageThunkData->u1.Function = (DWORD_PTR)lpFunction;
				#elif defined _M_X64
					lpImageThunkData->u1.Function = (DWORD_PTR)lpFunction;
				#endif

				if (VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &dwJunk))
					return lpOrgFunction;
			}

			lpImageThunkData++;
			lpImageOrgThunkData++;
		}

		lpImageImportDescriptor++;
	}

	return nullptr;
}

/**
 * Function to hook functions in the IAT of a the main module of the process.
 * \param lpModuleName : name of the module wich contains the function.
 * \param lpFunctionName : name of the function you want to hook.
 * \param lpFunction : pointer of the new function.
 * \return : the pointer of the original function or nullptr if it failed.
 */
LPVOID IAT::Hook(LPCSTR lpModuleName, LPCSTR lpFunctionName, const LPVOID lpFunction)
{
	const LPVOID hModule = GetCurrentProcessModule();
	const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)(hModule);
	if (lpImageDOSHeader == nullptr)
		return nullptr;

	const auto lpImageNtHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);

	const IMAGE_DATA_DIRECTORY ImportDataDirectory = lpImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	auto lpImageImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD_PTR)hModule + ImportDataDirectory.VirtualAddress);

	while (lpImageImportDescriptor->Characteristics != 0)
	{
		const auto lpCurrentModuleName = (LPSTR)((DWORD_PTR)lpImageDOSHeader + lpImageImportDescriptor->Name);
		if (strcmp(lpCurrentModuleName, lpModuleName) != 0)
		{
			lpImageImportDescriptor++;
			continue;
		}

		auto lpImageOrgThunkData = (PIMAGE_THUNK_DATA)((DWORD_PTR)lpImageDOSHeader + lpImageImportDescriptor->OriginalFirstThunk);
		auto lpImageThunkData = (PIMAGE_THUNK_DATA)((DWORD_PTR)lpImageDOSHeader + lpImageImportDescriptor->FirstThunk);

		while (lpImageOrgThunkData->u1.AddressOfData != 0)
		{
			const auto lpImportData = (PIMAGE_IMPORT_BY_NAME)((DWORD_PTR)lpImageDOSHeader + lpImageOrgThunkData->u1.AddressOfData);

			if (strcmp(lpFunctionName, lpImportData->Name) == 0)
			{
				DWORD dwJunk = 0;
				MEMORY_BASIC_INFORMATION mbi;

				VirtualQuery(lpImageThunkData, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
				if (!VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &mbi.Protect))
					return nullptr;

				const auto lpOrgFunction = (LPVOID)lpImageThunkData->u1.Function;

				#if defined _M_IX86
					lpImageThunkData->u1.Function = (DWORD_PTR)lpFunction;
				#elif defined _M_X64
					lpImageThunkData->u1.Function = (DWORD_PTR)lpFunction;
				#endif

				if (VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &dwJunk))
					return lpOrgFunction;
			}

			lpImageThunkData++;
			lpImageOrgThunkData++;
		}

		lpImageImportDescriptor++;
	}

	return nullptr;
}