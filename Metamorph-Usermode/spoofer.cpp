#include "spoofer.h"

#include "modules/windows.hpp"
#include "modules/scpsl.hpp"
#include "modules/steam.hpp"

#include "driver.h"

NTSTATUS Spoofer::LoadUsermode() {

	DWORDLONG Seed = time(NULL);
	srand(Seed);

	LOAD_MODULE(Windows);
	LOAD_MODULE(SCPSL);
	LOAD_MODULE(Steam);

	Success(XOR("Stopping services.."));

	Util::StopProcess(XOR("WmiPrvSE.exe"));
	
	std::system(XOR("vssadmin delete shadows /All /Quiet > nul 2>&1"));
	std::system(XOR("net stop winmgmt /Y > nul 2>&1"));

	return STATUS_SUCCESS;
}

NTSTATUS Spoofer::LoadKernelmode() {

	HANDLE iqvw64e_device_handle = intel_driver::Load();

	if (iqvw64e_device_handle == INVALID_HANDLE_VALUE) {
		Error(XOR("Failed to load vulnerable driver"));

		return STATUS_UNSUCCESSFUL;
	}

	Success(XOR("Loaded Vulnerable Driver"));

	NTSTATUS exitCode = 0;
	
	if (!kdmapper::MapDriver(iqvw64e_device_handle, (BYTE*)driver, 0, 0, false, true, kdmapper::AllocationMode::AllocateMdl, false, NULL, &exitCode)) {
		Error(XOR("Failed to map driver"));
	
		intel_driver::Unload(iqvw64e_device_handle);

		return STATUS_UNSUCCESSFUL;
	}

	Success(XOR("Mapped Spoofer"));

	if (!intel_driver::Unload(iqvw64e_device_handle)) {
		Error(XOR("Failed to fully unload vulnerable driver"));

		return STATUS_UNSUCCESSFUL;
	}

	Success(XOR("Unloaded Vulnerable Driver"));

	return STATUS_SUCCESS;
}

DWORD Spoofer::GetProcessPID(const char* processName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return 0;

    wchar_t wProcessName[MAX_PATH] = { 0 };
    size_t converted = 0;
    mbstowcs_s(&converted, wProcessName, MAX_PATH, processName, MAX_PATH - 1);

    PROCESSENTRY32W pe32 = { 0 };
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    DWORD pid = 0;
    if (Process32FirstW(hSnapshot, &pe32))
    {
        do
        {
            if (_wcsicmp(pe32.szExeFile, wProcessName) == 0)
            {
                pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return pid;
}

BOOL Spoofer::LaunchWithParent(
    const char* exePath,
    const char* workingDir,
    const char* parentProcess)
{
    DWORD parentPID = GetProcessPID(parentProcess);
    if (parentPID == 0)
    {
        printf("[!] Parent process not found: %s\n", parentProcess);
        return FALSE;
    }

    HANDLE hParent = OpenProcess(PROCESS_CREATE_PROCESS, FALSE, parentPID);
    if (!hParent)
    {
        printf("[!] Failed to open parent process. Error: %lu\n", GetLastError());
        printf("[!] Jalankan Metamorph sebagai Administrator!\n");
        return FALSE;
    }

    STARTUPINFOEXA siex = { 0 };
    siex.StartupInfo.cb = sizeof(STARTUPINFOEXA);

    SIZE_T attrSize = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attrSize);

    siex.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
        GetProcessHeap(), 0, attrSize);

    if (!siex.lpAttributeList)
    {
        CloseHandle(hParent);
        return FALSE;
    }

    if (!InitializeProcThreadAttributeList(siex.lpAttributeList, 1, 0, &attrSize))
    {
        HeapFree(GetProcessHeap(), 0, siex.lpAttributeList);
        CloseHandle(hParent);
        return FALSE;
    }

    if (!UpdateProcThreadAttribute(
        siex.lpAttributeList,
        0,
        PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
        &hParent,
        sizeof(HANDLE),
        NULL,
        NULL))
    {
        DeleteProcThreadAttributeList(siex.lpAttributeList);
        HeapFree(GetProcessHeap(), 0, siex.lpAttributeList);
        CloseHandle(hParent);
        return FALSE;
    }

    PROCESS_INFORMATION pi = { 0 };

    BOOL result = CreateProcessA(
        exePath,
        NULL,
        NULL,
        NULL,
        FALSE,
        EXTENDED_STARTUPINFO_PRESENT,
        NULL,
        workingDir,
        (LPSTARTUPINFOA)&siex,
        &pi
    );

    // Cleanup
    DeleteProcThreadAttributeList(siex.lpAttributeList);
    HeapFree(GetProcessHeap(), 0, siex.lpAttributeList);
    CloseHandle(hParent);

    if (result)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return result;
}