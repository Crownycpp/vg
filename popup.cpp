#include <iostream>
#include <windows.h>
#include <string>
#include <TlHelp32.h>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "User32.lib")


void Log(const std::string& message);

constexpr uint32_t JobObjectFreezeInformation = 18;

typedef struct _JOBOBJECT_WAKE_FILTER {
	ULONG HighEdgeFilter;
	ULONG LowEdgeFilter;
} JOBOBJECT_WAKE_FILTER, * PJOBOBJECT_WAKE_FILTER;

typedef struct _JOBOBJECT_FREEZE_INFORMATION {
	union {
		ULONG Flags;
		struct {
			ULONG FreezeOperation : 1;
			ULONG FilterOperation : 1;
			ULONG SwapOperation : 1;
			ULONG Reserved : 29;
		};
	};
	BOOLEAN Freeze;
	BOOLEAN Swap;
	UCHAR Reserved0[2];
	JOBOBJECT_WAKE_FILTER WakeFilter;
} JOBOBJECT_FREEZE_INFORMATION, * PJOBOBJECT_FREEZE_INFORMATION;

HANDLE globalJobHandle = NULL;

bool FreezeProcess(HANDLE hProcess) {
	globalJobHandle = CreateJobObject(NULL, NULL);
	if (!globalJobHandle) {
		std::cerr << "Failed to create Job Object. Error: " << GetLastError() << std::endl;
		return false;
	}

	if (!AssignProcessToJobObject(globalJobHandle, hProcess)) {
		std::cerr << "Failed to assign process to Job Object. Error: " << GetLastError() << std::endl;
		CloseHandle(globalJobHandle);
		globalJobHandle = NULL;
		return false;
	}

	JOBOBJECT_FREEZE_INFORMATION freezeInfo = { 0 };
	freezeInfo.FreezeOperation = 1;
	freezeInfo.Freeze = TRUE;

	if (!SetInformationJobObject(globalJobHandle, (JOBOBJECTINFOCLASS)JobObjectFreezeInformation, &freezeInfo, sizeof(freezeInfo))) {
		std::cerr << "Failed to freeze Job Object. Error: " << GetLastError() << std::endl;
		CloseHandle(globalJobHandle);
		globalJobHandle = NULL;
		return false;
	}

	return true;
}

bool ThawProcess(HANDLE hProcess) {
	if (!globalJobHandle) {
		std::cerr << "No valid job handle available for [removed]. Did you freeze the process first?" << std::endl;
		return false;
	}

	JOBOBJECT_FREEZE_INFORMATION freezeInfo = { 0 };
	freezeInfo.FreezeOperation = 1;
	freezeInfo.Freeze = FALSE;

	if (!SetInformationJobObject(globalJobHandle, (JOBOBJECTINFOCLASS)JobObjectFreezeInformation, &freezeInfo, sizeof(freezeInfo))) {
		std::cerr << "Failed to [removed] Object. Error: " << GetLastError() << std::endl;
		return false;
	}

	std::cout << "Process [removed] successfully!" << std::endl;
	return true;
}

DWORD GetServicePID(const wchar_t* serviceName) {
	DWORD pid = 0;

	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	if (!hSCManager) {
		std::wcerr << L"Error opening Service Control Manager: " << GetLastError() << std::endl;
		return 0;
	}

	SC_HANDLE hService = OpenService(hSCManager, serviceName, SERVICE_QUERY_STATUS);
	if (!hService) {
		std::wcerr << L"Error opening service: " << GetLastError() << std::endl;
		CloseServiceHandle(hSCManager);
		return 0;
	}

	SERVICE_STATUS_PROCESS ssp;
	DWORD bytesNeeded;
	if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &bytesNeeded)) {
		pid = ssp.dwProcessId;
	}
	else {
		std::wcerr << L"Error obtaining service status: " << GetLastError() << std::endl;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);

	return pid;
}

HANDLE GetProcessHandle(DWORD processId) {
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (!hProcess) {
		std::cerr << "Error opening process (PID: " << processId << "): " << GetLastError() << std::endl;
	}
	return hProcess;
}

void adjust_privileges() {
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;

	try {
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
			throw std::runtime_error("Failed to open process token.");
		}

		LUID luid;
		if (!LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege", &luid)) {
			throw std::runtime_error("Failed to look up privilege value.");
		}

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (!AdjustTokenPrivileges(hToken, FALSE, &tp, 0, nullptr, nullptr)) {
			throw std::runtime_error("Failed to adjust token privileges.");
		}

	}
	catch (const std::exception& e) {
		std::wstring error_message = L"Failed to adjust privileges: " + std::wstring(e.what(), e.what() + strlen(e.what()));
	}
}

int pb260605() {
	adjust_privileges();
	system("sc start vgc");


	system("cls");

	std::wcout << "\n [+] Waiting for VALORANT-Win64-Shipping.exe..." << std::endl;
	Log("Waiting for Valorant...");
	Sleep(50);

	uint32_t vgc_pid = 0;

	while (vgc_pid == 0) {
		PROCESSENTRY32W entry;
		entry.dwSize = sizeof(PROCESSENTRY32W);
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (Process32FirstW(snapshot, &entry)) {
			do {
				if (wcscmp(entry.szExeFile, L"VALORANT-Win64-Shipping.exe") == 0) {
					vgc_pid = entry.th32ProcessID;
					break;
				}
			} while (Process32NextW(snapshot, &entry));
		}
		CloseHandle(snapshot);
		Sleep(2000);
	}

	std::wcout << "\n [+] VALORANT-Win64-Shipping.exe found.\n\n  [+] Loading Bypass..." << std::endl;
	Log("Valorant found!");
	std::wcout << "\n [>] ETA: 45 Seconds.." << std::endl;

	Sleep(45000);

	const wchar_t* serviceName = L"Dnscache";
	DWORD pid = GetServicePID(serviceName);

	if (pid) {
		FreezeProcess(GetProcessHandle(pid));

		std::wcout << "\n  [>] Done!" << std::endl;
		std::wcout << "\n\n  [OK] Popup has been bypassed!" << std::endl;
		std::wcout << "\n  [!] DO NOT CLOSE THIS WINDOW!" << std::endl;
		std::wcout << "\n\n  [+] Press F2 to kill bypass safely." << std::endl;

		Log("Popup bypass active");

		Log("Press F2 to remove");

		while (true) {
			if (GetAsyncKeyState(VK_F2) & 0x8000) {
				std::wcout << "\n  [-] Removing bypass. Exiting..." << std::endl;
				ThawProcess(GetProcessHandle(pid));
				std::wcout << "\n  [+] Bypass removed. Exiting..." << std::endl;
				exit(0);
			}
			Sleep(20);
		}
	}

	return 0;
}