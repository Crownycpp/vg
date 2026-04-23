#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <iostream>
#include <sstream>
#include <string>
#include <regex>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <thread>
#include <chrono>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <atomic>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")


void Log(const std::string& message);

// ----------------------------------------------------------------
// Global state – kept exactly as the decompiled binary’s globals
// ----------------------------------------------------------------
namespace globals {
    HANDLE g_hPipe = nullptr;
    volatile bool g_bStop = false;        // Ctrl+C or exit trigger
    bool g_bData1 = false;
    bool g_bData2 = false;
    bool g_bData3 = false;                // guards the “stop vgc” Beep
    DWORD g_dwTlsIndex = 0;
}

std::mutex log_mutex;                     // serialise console output
const char* UUID_PATTERN = "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}";

// ----------------------------------------------------------------
// Console helpers (must be called at startup)
// ----------------------------------------------------------------
void EnableVirtualTerminal() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void SetConsoleSize() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT rect = { 0, 0, 120, 40 };
    SetConsoleWindowInfo(hOut, TRUE, &rect);
    COORD size = { 120, 9001 };
    SetConsoleScreenBufferSize(hOut, size);
}

// ----------------------------------------------------------------
// Logging – timestamp + coloured prefix, identical to decompiled version
// ----------------------------------------------------------------
enum LogLevel { LOG_NONE, LOG_OK, LOG_WARN, LOG_ERR, LOG_INFO };



// simpler overload used by the user’s existing code


// ----------------------------------------------------------------
// Hex dump (preserved from user’s code)
// ----------------------------------------------------------------
void HexDump(const void* data, size_t len, const char* title) {
    std::stringstream ss;
    ss << title << " Hex: ";
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < len; ++i) {
        if (i > 0 && i % 16 == 0) ss << "\n       ";
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)p[i] << " ";
    }

}

// ----------------------------------------------------------------
// UUID extraction – uses the same regex as the decompiled program
// ----------------------------------------------------------------
bool SearchUUID(const char* data, size_t len, std::vector<uint8_t>& uuidBin, char* uuidStr, size_t uuidStrSize) {
    std::regex uuid_regex(UUID_PATTERN, std::regex::icase | std::regex::ECMAScript);
    std::string input(data, len);
    std::smatch match;
    if (std::regex_search(input, match, uuid_regex)) {
        std::string uuid = match.str();
        strncpy_s(uuidStr, uuidStrSize, uuid.c_str(), _TRUNCATE);

        uuidBin.resize(16);
        size_t j = 0;
        for (size_t i = 0; i < uuid.length() && j < 16; ++i) {
            if (uuid[i] == '-') continue;
            unsigned int byte;
            if (sscanf_s(uuid.c_str() + i, "%2x", &byte) == 1) {
                uuidBin[j++] = (uint8_t)byte;
                ++i; // extra increment because we consumed two chars
            }
        }
        return true;
    }
    return false;
}

// ----------------------------------------------------------------
// Build response – exact protocol versions v1‑v5 from sub_140002bc0
// ----------------------------------------------------------------
void BuildResponse(int version, const std::vector<uint8_t>& uuid, std::vector<uint8_t>& out) {
    out.clear();
    // The decompiled code uses a magic that is the original magic + 1,
    // and the type field indicates the payload size.
    // For simplicity we reconstruct the exact packet structure.
    switch (version) {
    case 1: { // 0x100000028, 8 bytes payload
        out.resize(0x24);
        *(uint32_t*)&out[0] = 0;          // placeholder, will be set by caller
        *(uint32_t*)&out[4] = 0x100000028;
        *(uint32_t*)&out[8] = 8;
        break;
    }
    case 2: { // 0x100000028, 8 bytes payload, includes some of the UUID
        out.resize(0x24);
        *(uint32_t*)&out[4] = 0x100000028;
        *(uint32_t*)&out[8] = 8;
        if (!uuid.empty())
            memcpy(&out[0x24 - 8], uuid.data(), 8);
        break;
    }
    case 3: { // 0x100000038, 16 bytes payload
        out.resize(0x24);
        *(uint32_t*)&out[4] = 0x100000038;
        *(uint32_t*)&out[8] = 16;
        if (!uuid.empty())
            memcpy(&out[0x24 - 16], uuid.data(), 16);
        break;
    }
    case 4: { // 0x100000028, 8 bytes timestamp
        out.resize(0x24);
        *(uint32_t*)&out[4] = 0x100000028;
        *(uint32_t*)&out[8] = 8;
        uint64_t ticks = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        memcpy(&out[0x24 - 8], &ticks, 8);
        break;
    }
    case 5: { // 0x100000040, 24 bytes payload + timestamp
        out.resize(0x24);
        *(uint32_t*)&out[4] = 0x100000040;
        *(uint32_t*)&out[8] = 24;
        if (!uuid.empty())
            memcpy(&out[0x24 - 24], uuid.data(), 16);
        uint64_t ticks = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        memcpy(&out[0x24 - 8], &ticks, 8);
        break;
    }
    }
}

// ----------------------------------------------------------------
// Client handler thread – identical to sub_140003130
// ----------------------------------------------------------------
DWORD WINAPI ClientThread(LPVOID param) {
    HANDLE hPipe = (HANDLE)param;
    Log("NEW CONNECTION");

    const size_t BUFFER_SIZE = 0x4000;
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    std::vector<uint8_t> sendBuf;

    while (!globals::g_bStop) {
        DWORD bytesRead = 0;
        if (!ReadFile(hPipe, buffer.data(), BUFFER_SIZE, &bytesRead, nullptr) || bytesRead == 0) {
            break;
        }

        if (bytesRead < 12) continue;
        uint32_t magic = buffer[0];                       // original magic
        uint32_t type = *(uint32_t*)&buffer[4];          // packet type
        uint32_t size = *(uint32_t*)&buffer[8];          // data size

        char tmp[256];
        sprintf_s(tmp, "RECV  magic=0x%02X", magic, type, bytesRead);
        Log(tmp);

        sendBuf.clear();
        std::vector<uint8_t> uuidBin;
        char uuidStr[128] = { 0 };

        if (type == 1) {
            // Heartbeat / unknown – echo back with magic+1
            sendBuf = buffer;
            if (!sendBuf.empty()) sendBuf[0] = magic + 1;
        }
        else if (type == 2) {
            // Server list request – this causes “stop vgc” and Beep only once
            Log("Server list request – stopping VGC...");
            sendBuf.resize(0x24);
            sendBuf[0] = magic + 1;
            *(uint32_t*)&sendBuf[4] = 0x28;   // type
            *(uint32_t*)&sendBuf[8] = 1;
            if (!globals::g_bData3) {
                system("sc stop vgc >nul 2>&1");
                Beep(1000, 300);
                globals::g_bData3 = true;
            }
        }
        else if (type == 4) {
            // Auth token packet
            Log("Auth token");
            int version = 1;
            if (SearchUUID((char*)buffer.data(), bytesRead, uuidBin, uuidStr, sizeof(uuidStr))) {
                sprintf_s(tmp, "UUID extracted: %s", uuidStr);

                HexDump(uuidBin.data(), uuidBin.size(), "UUID");
                version = 4;   // use v4 when UUID found
            }
            else {
                Log("UUID error");
            }
            BuildResponse(version, uuidBin, sendBuf);
            // Set the correct magic in the first dword
            *(int*)sendBuf.data() = magic + 1;
        }
        else {
            // Unknown – just echo
            sendBuf = buffer;
            if (!sendBuf.empty()) sendBuf[0] = magic + 1;
        }

        if (!sendBuf.empty()) {
            DWORD written = 0;
            WriteFile(hPipe, sendBuf.data(), (DWORD)sendBuf.size(), &written, nullptr);
            sprintf_s(tmp, "Response dispatched  (%lu bytes)", written);

        }
        Sleep(10);
    }

    CloseHandle(hPipe);

    return 0;
}

// ----------------------------------------------------------------
// Pipe server – creates the named pipe, accepts one client at a time
// ----------------------------------------------------------------
DWORD WINAPI PipeServerThread(LPVOID) {
    while (!globals::g_bStop) {
        HANDLE hPipe = CreateNamedPipeW(
            L"\\\\.\\pipe\\933823D3-C77B-4BAE-89D7-A92B567236BC",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            0x100000, 0x100000, 500, nullptr);

        if (hPipe == INVALID_HANDLE_VALUE) {
            Log("Init Failed");
            Sleep(1000);
            continue;
        }

        Log("Init Sucess");

        if (ConnectNamedPipe(hPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
            Log("Client connected");
            HANDLE hThread = CreateThread(nullptr, 0, ClientThread, hPipe, 0, nullptr);
            if (hThread) {
                CloseHandle(hThread); // fire‑and‑forget
            }
            else {
                CloseHandle(hPipe);
            }
        }
        else {
            CloseHandle(hPipe);
        }
    }
    return 0;
}

// ----------------------------------------------------------------
// Loading animation (spinning line while waiting for Valorant)
// ----------------------------------------------------------------
void LoadingAnimation() {
    const char* spinners = "-\\|/";
    int i = 0;
    while (!globals::g_bStop) {
        if (globals::g_bStop) break;
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "\r  \x1b[93m" << spinners[i % 4]
            << " \x1b[1m\x1b[97mWaiting for Valorant...\x1b[0m   " << std::flush;
        Sleep(100);
        ++i;
    }
    // Clear the line
    std::cout << "\r                                              \r" << std::flush;
}

// ----------------------------------------------------------------
// Ctrl+C handler – clean shutdown
// ----------------------------------------------------------------
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        globals::g_bStop = true;
        return TRUE;
    }
    return FALSE;
}

// ----------------------------------------------------------------
// Main entry point – entire workflow exactly as the original
// ----------------------------------------------------------------
int m37286326273() {
    // 1. Console setup
    EnableVirtualTerminal();
    SetConsoleSize();
    SetConsoleTitleA("UXI VGC EMULATOR - BY SAMARPIT");
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    system("cls");   // clear screen once
    std::cout << "\x1b[92m\x1b[1m    UX-I VGC EMULATOR    v67.0\x1b[0m\n\n";

    // 2. Initialise

    Log("INITIALIZING");

    system("sc stop vgc >nul 2>&1");
    Sleep(500);
    Log("Starting VGC service...");
    system("sc start vgc >nul 2>&1");
    Sleep(500);

    // 3. Override the original pipe
    HANDLE testPipe = CreateFileW(
        L"\\\\.\\pipe\\933823D3-C77B-4BAE-89D7-A92B567236BC",
        GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (testPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(testPipe);
        Log("Step 1 success!");
    }

    // 4. Start pipe server in a background thread
    CreateThread(nullptr, 0, PipeServerThread, nullptr, 0, nullptr);

    // 5. Show loading animation while waiting for Valorant
    std::thread animThread(LoadingAnimation);
    animThread.detach();

    Log("Waiting for Valorant...");
    while (!globals::g_bStop) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe = { sizeof(pe) };
            if (Process32FirstW(hSnapshot, &pe)) {
                do {
                    if (_wcsicmp(pe.szExeFile, L"VALORANT-Win64-Shipping.exe") == 0) {
                        CloseHandle(hSnapshot);
                        globals::g_bStop = false;   // reset stop flag (we just used Ctrl-C as a break)
                        Log("Valorant detected!");
                        // The loading animation thread will exit because we temporarily set stop?
                        // We'll join the animation here by setting a flag, but since we detach we just let it die.
                        // The original sets animationActive = false; here we'll just log.
                        goto valorant_found;
                    }
                } while (Process32NextW(hSnapshot, &pe));
            }
            CloseHandle(hSnapshot);
        }
        Sleep(500);
    }

valorant_found:


    while (!globals::g_bStop) {
        Sleep(1000);
    }

    // 7. Shutdown
    Log("SHUTDOWN");
    Log("Shutting down");
    Sleep(500);
    return 0;
}