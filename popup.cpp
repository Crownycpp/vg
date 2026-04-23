// Popup bypass - By agre
// Reconstructed from decompiled binary (fixed)

#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <process.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "ntdll.lib")
void Log(const std::string& message);
// ------------------------------------------------------------------
// Custom vsnprintf_s wrapper
// ------------------------------------------------------------------
int __cdecl vsnprintf_s_wrapper(char* buffer, size_t bufferCount, size_t maxCount, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, bufferCount, format, args);
    va_end(args);
    if (result < 0) return -1;
    return result;
}

// ------------------------------------------------------------------
// Console title and output helpers
// ------------------------------------------------------------------
const wchar_t ConsoleTitle[] = L"  Popup bypass - By agre ";

// ------------------------------------------------------------------
// Function to run a command via cmd.exe and wait for it to finish
// ------------------------------------------------------------------
BOOL RunCommandWait(const char* command) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    char cmdLine[1024];
    vsnprintf_s_wrapper(cmdLine, sizeof(cmdLine), sizeof(cmdLine), "cmd.exe /c %s", command);

    if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return TRUE;
    }
    return FALSE;
}

// ------------------------------------------------------------------
// Function to run a command via cmd.exe without waiting
// ------------------------------------------------------------------
BOOL RunCommandNoWait(const char* command) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    char cmdLine[1024];
    vsnprintf_s_wrapper(cmdLine, sizeof(cmdLine), sizeof(cmdLine), "%s", command);

    if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return TRUE;
    }
    return FALSE;
}

// ------------------------------------------------------------------
// Dynamic function pointers for NtSuspendProcess / NtResumeProcess
// ------------------------------------------------------------------
typedef NTSTATUS(NTAPI* NtSuspendProcess_t)(HANDLE ProcessHandle);
typedef NTSTATUS(NTAPI* NtResumeProcess_t)(HANDLE ProcessHandle);

NtSuspendProcess_t NtSuspendProcess = nullptr;
NtResumeProcess_t NtResumeProcess = nullptr;

DWORD dwProcessId = 0;          // PID of the Dnscache service
HWND  g_hwnd = NULL;            // handle of the console window
bool  g_running = true;         // main loop flag

// ------------------------------------------------------------------
// Suspend/resume the DnsCache process (bypassed popup)
// ------------------------------------------------------------------
void SuspendDnsCache() {
    if (dwProcessId && NtSuspendProcess) {
        HANDLE hProc = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, dwProcessId);
        if (hProc) {
            NtSuspendProcess(hProc);
            CloseHandle(hProc);
        }
    }
}

void ResumeDnsCache() {
    if (dwProcessId && NtResumeProcess) {
        HANDLE hProc = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, dwProcessId);
        if (hProc) {
            NtResumeProcess(hProc);
            CloseHandle(hProc);
        }
    }
}

// ------------------------------------------------------------------
// EnumWindows callback to look for the VALORANT window
// ------------------------------------------------------------------
BOOL CALLBACK EnumFunc(HWND hWnd, LPARAM lParam) {
    char windowTitle[256] = { 0 };
    GetWindowTextA(hWnd, windowTitle, sizeof(windowTitle));
    if (strstr(windowTitle, "VALORANT  ")) {
        *(bool*)lParam = true;
        return FALSE;   // stop enumeration
    }
    return TRUE;
}

// ------------------------------------------------------------------
// Delete firewall rules that block vgc.exe / vgm.exe
// ------------------------------------------------------------------
BOOL DeleteFirewallRules() {
    RunCommandWait("netsh advfirewall firewall delete rule name=\"Block vgc.exe\"");
    return RunCommandWait("netsh advfirewall firewall delete rule name=\"Block vgm.exe\"");
}

// ------------------------------------------------------------------
// Safe exit: hide console, kill Riot processes, resume DNS, etc.
// ------------------------------------------------------------------
void SafeExit() {
    std::cout << "\x1B[1;31m  [+] Safe Exit ...\x1B[1;97m" << std::endl;
    ShowWindow(g_hwnd, SW_HIDE);

    const char* procList[] = { "Riot", "vgc", "VALORANT", "CrashReport" };
    for (auto& name : procList) {
        char buffer[128];
        vsnprintf_s_wrapper(buffer, sizeof(buffer), sizeof(buffer), "taskkill /F /IM %s*", name);
        RunCommandWait(buffer);
    }

    ResumeDnsCache();
    RunCommandWait("w32tm /resync");
    RunCommandWait("sc start vgc");
    DeleteFirewallRules();
    ExitProcess(0);
}

// ------------------------------------------------------------------
// Console control handler (Ctrl+C, etc.)
// ------------------------------------------------------------------
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_CLOSE_EVENT || dwCtrlType == CTRL_C_EVENT) {
        SafeExit();
        return TRUE;
    }
    return FALSE;
}

// ------------------------------------------------------------------
// Hotkey monitoring thread (F10 -> exit)
// ------------------------------------------------------------------
unsigned __stdcall HotkeyThread(void*) {
    while (g_running) {
        if (GetAsyncKeyState(VK_F10) & 0x8000) {
            SafeExit();
            Sleep(500);
        }
        Sleep(50);
    }
    return 0;
}

// ------------------------------------------------------------------
// Main entry point
// ------------------------------------------------------------------
int pb260605() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hConsole, &mode);
    SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleTitleW(ConsoleTitle);

    // Set console buffer and window size (fixed narrowing conversion)
    COORD bufferSize;
    bufferSize.X = static_cast<SHORT>(19660857 & 0xFFFF);   // low‑16 bits
    bufferSize.Y = static_cast<SHORT>((19660857 >> 16) & 0xFFFF); // high‑16 bits
    SetConsoleScreenBufferSize(hConsole, bufferSize);

    SMALL_RECT windowRect = { 0, 0, 1245240, 1 };
    SetConsoleWindowInfo(hConsole, TRUE, &windowRect);

    g_hwnd = GetConsoleWindow();

    // If vgc.exe exists, grab its icon and set it for the console
    if (GetFileAttributesW(L"C:\\Program Files\\Riot Vanguard\\vgc.exe") != INVALID_FILE_ATTRIBUTES) {
        HICON hIcon = ExtractIconW(GetModuleHandleW(NULL), L"C:\\Program Files\\Riot Vanguard\\vgc.exe", 0);
        if (hIcon >= (HICON)2) {
            SendMessageW(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessageW(g_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
    }

    // Make console layered (semi-transparent)
    SetWindowPos(g_hwnd, HWND_MESSAGE, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE);
    LONG exStyle = GetWindowLongW(g_hwnd, GWL_EXSTYLE);
    SetWindowLongW(g_hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
    SetLayeredWindowAttributes(g_hwnd, 0, 230, LWA_ALPHA);

    // Enable SeDebugPrivilege
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        TOKEN_PRIVILEGES tp = { 0 };
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (LookupPrivilegeValueA(NULL, "SeDebugPrivilege", &tp.Privileges[0].Luid)) {
            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
            if (GetLastError() != ERROR_SUCCESS) {
                CloseHandle(hToken);
                std::cout << "\x1B[1;31m  [!] Need run as Administrator.\x1B[1;97m" << std::endl;
            }
            else {
                CloseHandle(hToken);
            }
        }
        else {
            CloseHandle(hToken);
            std::cout << "\x1B[1;31m  [!] Need run as Administrator.\x1B[1;97m" << std::endl;
        }
    }
    else {
        std::cout << "\x1B[1;31m  [!] Need run as Administrator.\x1B[1;97m" << std::endl;
    }

    // Dynamically resolve NtSuspendProcess / NtResumeProcess from ntdll
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (hNtdll) {
        NtSuspendProcess = (NtSuspendProcess_t)GetProcAddress(hNtdll, "NtSuspendProcess");
        NtResumeProcess = (NtResumeProcess_t)GetProcAddress(hNtdll, "NtResumeProcess");
    }

    // Set console control handler
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

    // Start hotkey thread
    uintptr_t hThread = _beginthreadex(NULL, 0, HotkeyThread, NULL, 0, NULL);
    if (hThread == 0) {
        std::cerr << "Failed to create hotkey thread!" << std::endl;
        return 1;
    }
    // Detach the thread by closing its handle (replaces undefined Thrd_detach)
    CloseHandle(reinterpret_cast<HANDLE>(hThread));

    // Obtain DnsCache service PID
    SC_HANDLE hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCM) {
        SC_HANDLE hService = OpenServiceW(hSCM, L"Dnscache", SERVICE_QUERY_STATUS);
        if (hService) {
            SERVICE_STATUS_PROCESS ssp = { 0 };
            DWORD bytesNeeded = 0;
            if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &bytesNeeded)) {
                dwProcessId = ssp.dwProcessId;
            }
            CloseServiceHandle(hService);
        }
        else {
            dwProcessId = 0;
        }
        CloseServiceHandle(hSCM);
    }
    else {
        dwProcessId = 0;
    }

    if (dwProcessId == 0) {
        std::cout << "\x1B[1;31m  [!] DNS Cache service not found.\x1B[1;97m" << std::endl;
    }

    // Ensure the DnsCache process is running (resume if needed)
    if (NtResumeProcess && dwProcessId) {
        HANDLE hProc = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, dwProcessId);
        if (hProc) {
            NtResumeProcess(hProc);
            CloseHandle(hProc);
        }
    }



    std::cout << "\x1B[1;32m  [+] Starting Valorant ...\x1B[1;97m" << std::endl;
    Log("Starting Valorant...");

    RunCommandWait("w32tm /resync");
    RunCommandWait("Del /F /S /Q \"C:\\Program Files\\Riot Vanguard\\Logs\\*\"");
    RunCommandNoWait("cmd.exe /c sc start vgc");
    RunCommandNoWait("\"C:\\Riot Games\\Riot Client\\RiotClientServices.exe\" --launch-product=valorant --launch-patchline=live");

    // Main monitoring loop
    bool valorantFound = false;
    bool initBypassed = false;
    bool threadSuspended = false;

    while (g_running) {
        // Enumerate processes to find vgc.exe
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        DWORD vgcPid = 0;
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe = { sizeof(pe) };
            if (Process32FirstW(hSnapshot, &pe)) {
                do {
                    if (_wcsicmp(pe.szExeFile, L"vgc.exe") == 0) {
                        vgcPid = pe.th32ProcessID;
                        break;
                    }
                } while (Process32NextW(hSnapshot, &pe));
            }
            CloseHandle(hSnapshot);
        }

        // Check if VALORANT window exists
        bool windowFound = false;
        EnumWindows(EnumFunc, (LPARAM)&windowFound);

        if (windowFound != valorantFound) {
            valorantFound = windowFound;
            if (!windowFound) {
                system("cls");

                std::cout << "\x1B[1;33m  [*] Waiting Valorant ...\x1B[1;97m" << std::flush;
                // Re-enable DNS cache while waiting
                if (dwProcessId && NtResumeProcess) {
                    HANDLE hProc = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, dwProcessId);
                    if (hProc) {
                        NtResumeProcess(hProc);
                        CloseHandle(hProc);
                    }
                }
                RunCommandWait("w32tm /resync");
                RunCommandWait("sc start vgc");
                threadSuspended = false;
                initBypassed = true;
            }
            else {
                std::cout << "\x1B[1;32m  [+] Valorant found.                     \x1B[1;97m" << std::endl;
                Log("´Valorant found");
            }
        }


        if (windowFound && vgcPid && (!initBypassed || !threadSuspended)) {
            std::cout << "\r\x1B[1;33m  [*] Waiting VGC Init to Bypass PopUp...\x1B[1;97m                    " << std::flush;
            Log("Waiting for VGC...");
            while (g_running) {
                HANDLE hThSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
                int cThreads = 0;
                if (hThSnapshot != INVALID_HANDLE_VALUE) {
                    THREADENTRY32 te = { sizeof(te) };
                    if (Thread32First(hThSnapshot, &te)) {
                        do {
                            if (te.th32OwnerProcessID == vgcPid)
                                cThreads++;
                        } while (Thread32Next(hThSnapshot, &te));
                    }
                    CloseHandle(hThSnapshot);
                }
                if (cThreads > 14)   // VGC has spawned enough threads
                    break;
                Sleep(10);
            }
            Sleep(200);

            // Suspend DNS cache to bypass VAN -102 popup
            if (dwProcessId && NtSuspendProcess) {
                HANDLE hProc = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, dwProcessId);
                if (hProc) {
                    NtSuspendProcess(hProc);
                    CloseHandle(hProc);
                }
            }
            threadSuspended = true;
            initBypassed = true;
            std::cout << "\r\x1B[1;32m  [+] Popup Bypassed ! \x1B[1;33m ( VAN -102 )\x1B[1;97m                    " << std::endl;
            Log("Bypass sucess!");
        }

        Sleep(500);
    }

    return 0;
}