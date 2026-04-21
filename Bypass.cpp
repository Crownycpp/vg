
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <shlobj.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <mutex>
#include <intrin.h>
extern std::atomic<DWORD> g_VanguardStatus;
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
void Log(const std::string& message);
// -------------------- Global Constants (original obfuscated values resolved) --------------------
constexpr uint8_t  c1 = 0, c2 = 2, c3 = 3, c6 = 6, c7 = 7, c9 = 9;
constexpr uint8_t  B0 = 8, B1 = 9, B2 = 7, B3 = 1, B4 = 3;   // byte_1401CB5E0..E4
constexpr double   delta = (double)B2 - ((double)B4 + (double)B4);  // 7 - (3+3) = 1.0

// Global state
HANDLE g_hConsole = nullptr;
std::mutex g_consoleMutex;
bool g_vanguardActive = false;
DWORD g_vgcPid = 0;
int  g_spoofStage = 0;

// -------------------- Helper: Execute system command silently --------------------
void RunSilent(const std::string& cmd) {
    std::string full = cmd + " > nul 2>&1";
    system(full.c_str());
}

// -------------------- Console Window Setup (deobfuscated) --------------------
void SetupConsoleWindow() {
    HWND hwnd = GetConsoleWindow();
    HANDLE hStd = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleW(L"Vanguard Utility");
    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    SetWindowLongA(hwnd, GWL_STYLE, style & ~(WS_CAPTION | WS_THICKFRAME));
    SetWindowLongA(hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, 153, LWA_ALPHA);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStd, &csbi);
    COORD size = { csbi.srWindow.Right - csbi.srWindow.Left + 1,
                   csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };
    SetConsoleScreenBufferSize(hStd, size);
    ShowScrollBar(hwnd, SB_BOTH, FALSE);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    MoveWindow(hwnd, (screenW - 400) / 2, (screenH - 350) / 2, 400, 350, TRUE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hStd, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hStd, &cursorInfo);
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
}

// -------------------- Enable SeDebugPrivilege --------------------
void EnableDebugPrivilege() {
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        LUID luid;
        LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege", &luid);
        TOKEN_PRIVILEGES tp = { 1, { { luid, SE_PRIVILEGE_ENABLED } } };
        AdjustTokenPrivileges(hToken, FALSE, &tp, 0, nullptr, nullptr);
        CloseHandle(hToken);
    }
}

// -------------------- Get Process ID by Name (simplified) --------------------
DWORD GetProcessIdByName(const std::wstring& name) {
    DWORD pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (name == pe.szExeFile) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
    return pid;
}

// -------------------- Suspend Process via NtSuspendProcess --------------------
void SuspendProcess(DWORD pid) {
    static auto pNtSuspendProcess = (LONG(NTAPI*)(HANDLE))GetProcAddress(GetModuleHandleA("ntdll"), "NtSuspendProcess");
    if (pNtSuspendProcess) {
        HANDLE hProc = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
        if (hProc) {
            pNtSuspendProcess(hProc);
            CloseHandle(hProc);
        }
    }
}

// -------------------- Write File to Disk (from embedded data) --------------------
bool WriteEmbeddedFile(const std::wstring& path, const void* data, size_t size) {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written;
    WriteFile(hFile, data, (DWORD)size, &written, nullptr);
    CloseHandle(hFile);
    return written == size;
}

// -------------------- Obfuscated Constant Strings (reconstructed) --------------------
const wchar_t* const g_strSpooferRunning = L"[*] HWID Spoofer & Cleaner baslatiliyor...";
const wchar_t* const g_strSpooferDone = L"[+] Spoofer islemi bitti.";
const wchar_t* const g_strHookLoaderStart = L"[*] Hook Loader baslatiliyor...";
const wchar_t* const g_strHookActive = L"[+] Hook aktif edildi.";
const wchar_t* const g_strVanguardBypass = L"[*] Vanguard Bypass Active";
const wchar_t* const g_strSafeExit = L"[*] Safe Exit initiated...";
const wchar_t* const g_strPopupBypassed = L"\n[+] Popup Bypassed active!";



// -------------------- HWID Spoofer Main --------------------
void RunHwidSpoofer() {

}

// -------------------- Vanguard Bypass (Phase 1 & 2) --------------------
void VanguardBypass() {

    Log("Waiting for Valorant");
    DWORD valorantPid = 0;
    while (!valorantPid) {
        valorantPid = GetProcessIdByName(L"VALORANT-Win64-Shipping.exe");
        if (GetAsyncKeyState(VK_F4) & 0x8000) return; // safe exit
        Sleep(500);
    }
    Log("Found Valorant");
    std::wcout << L"[+] VALORANT-RUNNING\n";
    MessageBoxW(nullptr, L"Press OK after starting a match!", L"vgc.exe", MB_ICONINFORMATION | MB_SYSTEMMODAL);
    Log("Bypass active");
    g_VanguardStatus = 3;
    // Firewall block rules
    RunSilent("netsh advfirewall firewall add rule name=\"VGC_BYP\" dir=out action=block program=\"C:\\Program Files\\Riot Vanguard\\vgc.exe\" enable=yes");
    RunSilent("netsh advfirewall firewall add rule name=\"VGC_BYP_IN\" dir=in action=block program=\"C:\\Program Files\\Riot Vanguard\\vgc.exe\" enable=yes");
    RunSilent("netsh advfirewall firewall add rule name=\"VGM_BYP\" dir=out action=block program=\"C:\\Program Files\\Riot Vanguard\\vgmtray.exe\" enable=yes");
    RunSilent("netsh advfirewall firewall add rule name=\"VGM_BYP_IN\" dir=in action=block program=\"C:\\Program Files\\Riot Vanguard\\vgmtray.exe\" enable=yes");

    // Suspend Vanguard threads
    DWORD vgcPid = GetProcessIdByName(L"vgc.exe");
    if (vgcPid) {
        SuspendProcess(vgcPid);
    }
    Log("Inject your cheat");

    Sleep(5000);
    MessageBoxW(nullptr, L"Press OK to stop bypass", L"vgc.exe", MB_ICONWARNING | MB_SYSTEMMODAL);


    RunSilent("net stop vgc /y");
    RunSilent("netsh advfirewall firewall delete rule name=\"VGC_BYP\"");
    RunSilent("netsh advfirewall firewall delete rule name=\"VGC_BYP_IN\"");
    RunSilent("netsh advfirewall firewall delete rule name=\"VGM_BYP\"");
    RunSilent("netsh advfirewall firewall delete rule name=\"VGM_BYP_IN\"");
    RunSilent("taskkill /F /IM VALORANT-Win64-Shipping.exe");
    g_VanguardStatus = 1;
    std::wcout << L"[*] Safe Exit initiated...\n";
}





void FixVan68() {
    RunSilent("netsh advfirewall reset");
    std::cout << "Fixed Van 68" << std::endl;
}


int b220824() {

    system("taskkill /F /IM VALORANT-Win64-Shipping.exe /T");


    system("taskkill /F /IM vgc.exe");

    EnableDebugPrivilege();

    while (true) {

        VanguardBypass();
    }
}