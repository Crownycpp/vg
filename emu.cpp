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
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#include <mutex>
#include <atomic>


extern std::atomic<DWORD> g_VanguardStatus;
void Log(const std::string& message);

namespace globals {
    HANDLE g_hPipe = nullptr;
    volatile bool g_bStop = false;
    bool g_bData1 = false;
    bool g_bData2 = false;
    bool g_bData3 = false;
    DWORD g_dwTlsIndex = 0;

}
std::mutex log_mutex;

const char* UUID_PATTERN = "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}";



void HexDump(const void* data, size_t len, const char* title);
bool SearchUUID(const char* data, size_t len, std::vector<uint8_t>& uuidBin, char* uuidStr, size_t uuidStrSize);
void BuildResponse(int version, const std::vector<uint8_t>& uuid, std::vector<uint8_t>& out);
DWORD WINAPI ClientThread(LPVOID param);
DWORD WINAPI PipeServerThread(LPVOID param);








void HSVtoRGB(float H, float S, float V, int& R, int& G, int& B) {
    float C = V * S;
    float X = C * (1.0f - std::abs(std::fmod(H / 60.0f, 2.0f) - 1.0f));
    float m = V - C;
    float r = 0, g = 0, b = 0;

    if (H >= 0 && H < 60) { r = C; g = X; b = 0; }
    else if (H >= 60 && H < 120) { r = X; g = C; b = 0; }
    else if (H >= 120 && H < 180) { r = 0; g = C; b = X; }
    else if (H >= 180 && H < 240) { r = 0; g = X; b = C; }
    else if (H >= 240 && H < 300) { r = X; g = 0; b = C; }
    else { r = C; g = 0; b = X; }

    R = static_cast<int>((r + m) * 255.0f);
    G = static_cast<int>((g + m) * 255.0f);
    B = static_cast<int>((b + m) * 255.0f);
}





void HexDump(const void* data, size_t len, const char* title) {
    std::stringstream ss;
    ss << title << " Hex: ";
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < len; ++i) {
        if (i > 0 && i % 16 == 0) ss << "\n       ";
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)p[i] << " ";
    }

}


void SetConsoleSize() {

    system("mode con: cols=60 lines=25");


    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}


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
                ++i; 
            }
        }
        return true;
    }
    return false;
}

// ------------------------------------------------------------------
// Build response packet (sub_140002bc0)
void BuildResponse(int version, const std::vector<uint8_t>& uuid, std::vector<uint8_t>& out) {
    out.clear();
    uint8_t magic = (version == 1) ? 0x28 : 0x40; // approximate from decomp
    uint32_t type = 2; 


    if (version == 1) {
        size_t size = 8 + 16; // header + UUID
        out.resize(size);
        out[0] = magic;
        *(uint32_t*)(&out[4]) = (uint32_t)(size - 8);
        memcpy(&out[8], uuid.data(), 16);
    }
    else if (version == 2) {
        size_t size = 8 + 8;
        out.resize(size);
        out[0] = magic;
        *(uint32_t*)(&out[4]) = (uint32_t)(size - 8);

        memset(&out[8], 0, 8);
    }
    else if (version == 3) {
        size_t size = 8 + 16;
        out.resize(size);
        out[0] = magic;
        *(uint32_t*)(&out[4]) = (uint32_t)(size - 8);
        memcpy(&out[8], uuid.data(), 16);
    }
    else if (version == 4) {
        size_t size = 8 + 8;
        out.resize(size);
        out[0] = magic;
        *(uint32_t*)(&out[4]) = (uint32_t)(size - 8);
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        memcpy(&out[8], &timestamp, 8);
    }
    else if (version == 5) {
        size_t size = 8 + 24;
        out.resize(size);
        out[0] = magic;
        *(uint32_t*)(&out[4]) = (uint32_t)(size - 8);
        memcpy(&out[8], uuid.data(), 16);
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        memcpy(&out[24], &timestamp, 8);
    }
}


DWORD WINAPI ClientThread(LPVOID param) {
    HANDLE hPipe = (HANDLE)param;
    Log("NEW CONNECTION");

    const size_t BUFFER_SIZE = 0x4000;
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    std::vector<uint8_t> sendBuf;

    while (!globals::g_bStop) {
        DWORD bytesRead = 0;
        if (!ReadFile(hPipe, buffer.data(), BUFFER_SIZE, &bytesRead, nullptr) || bytesRead == 0)
            break;


        if (bytesRead < 12) continue;
        uint8_t magic = buffer[0];
        uint32_t type = *(uint32_t*)(&buffer[4]);   
        uint32_t size = *(uint32_t*)(&buffer[8]);

        char tmp[256];
        sprintf_s(tmp, "RECV: magic=0x%02X, type=%u, size=%u", magic, type, size);


        sendBuf.clear();
        std::vector<uint8_t> uuidBin;
        char uuidStr[128] = { 0 };

        if (type == 1) {
            Log("RECV");
            sendBuf = buffer;
            if (!sendBuf.empty()) sendBuf[0]++;
        }
        else if (type == 2) {
            Log("RQ");
            sendBuf.resize(8);
            sendBuf[0] = magic + 1;
            *(uint32_t*)(&sendBuf[4]) = 0x28;
            if (!globals::g_bData3) {
                system("sc stop vgc >nul 2>&1");
                Beep(1000, 500);
                globals::g_bData3 = true;
            }
        }
        else if (type == 4) {
            Log("TOKEN SENT");
            int version = 1;
            if (SearchUUID((char*)buffer.data(), bytesRead, uuidBin, uuidStr, sizeof(uuidStr))) {
                sprintf_s(tmp, "Found UUID: %s", uuidStr);
                Log(tmp);
                HexDump(uuidBin.data(), uuidBin.size(), "UUID binary:");
                version = 4; 
            }
            else {
                Log("UUID NOT found");
            }
            BuildResponse(version, uuidBin, sendBuf);
        }
        else {
            sendBuf = buffer;
            if (!sendBuf.empty()) sendBuf[0]++;
        }


        if (!sendBuf.empty()) {
            DWORD written = 0;
            WriteFile(hPipe, sendBuf.data(), (DWORD)sendBuf.size(), &written, nullptr);
            Log("ACK");

        }
        Sleep(10);
    }

    CloseHandle(hPipe);
    Log("VALORANT CLOSED");
    g_VanguardStatus = 1;

    return 0;
}


// Pipe server thread (sub_1400034a0)
DWORD WINAPI PipeServerThread(LPVOID) {
    Log("Waiting for client...");
    while (!globals::g_bStop) {
        HANDLE hPipe = CreateNamedPipeW(
            L"\\\\.\\pipe\\933823D3-C77B-4BAE-89D7-A92B567236BC",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            0x100000, 0x100000, 500, nullptr);

        if (hPipe == INVALID_HANDLE_VALUE) {
            Sleep(1000);
            continue;
        }

        if (ConnectNamedPipe(hPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
            Log("Client connected!");
            HANDLE hThread = CreateThread(nullptr, 0, ClientThread, hPipe, 0, nullptr);
            if (hThread) {
                CloseHandle(hThread);
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


void ClearScreenThread() {

    while (!globals::g_bStop) {

        std::this_thread::sleep_for(std::chrono::seconds(90));


        if (globals::g_bStop) break;


        system("cls");


    }
}

void StartDetachedCleaner() {
    std::thread t(ClearScreenThread);
    t.detach(); 
}

int m37286326273() {


   


    system("taskkill /F /IM VALORANT-Win64-Shipping.exe /T");


    system("taskkill /F /IM vgc.exe");

        system("sc stop vgc >nul 2>&1");
        Sleep(500);
        system("sc start vgc >nul 2>&1");
        Sleep(500);

        HANDLE hPipe = CreateFileW(
            L"\\\\.\\pipe\\933823D3-C77B-4BAE-89D7-A92B567236BC",
            GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
            Log("VGC Connected");
        }

        char verMsg[64];
        Log(verMsg);


        CreateThread(nullptr, 0, PipeServerThread, nullptr, 0, nullptr);

        Log("Waiting for Valorant...");
        while (!globals::g_bStop) {
            HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnapshot != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32W pe = { sizeof(pe) };
                if (Process32FirstW(hSnapshot, &pe)) {
                    do {
                        if (_wcsicmp(pe.szExeFile, L"VALORANT-Win64-Shipping.exe") == 0) {
                            CloseHandle(hSnapshot);
                            Log("Valorant detected!");
                            StartDetachedCleaner();
                            goto valorant_found;
                        }
                    } while (Process32NextW(hSnapshot, &pe));
                }
                CloseHandle(hSnapshot);
            }
            Sleep(500);
        }


    valorant_found:
        g_VanguardStatus = 2;
        while (!globals::g_bStop) {
            Sleep(1000);
        }
        Log("Shutting down...");

}