#define WIN32_LEAN_AND_MEAN
#define STB_IMAGE_IMPLEMENTATION
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include "image.h"
#include "stb_image.h"
#include <dwmapi.h>
#include <d3d11.h>
#include <tchar.h>
#include <vector>
#include <string>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "vexyl.hpp"
#include "Bypass.h"
#include <thread>
#include <atomic>
#include "font.h"
#include "headerfont.h"
#include "emu.h"
#include "popup.h"
std::atomic<int> g_UplinkPing{ 0 };
#include <thread>
#include <atomic>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#include <map>


static std::map<ImGuiID, float> hover_anim;
static std::map<ImGuiID, float> click_anim;

bool AnimatedButton(const char* label, ImVec2 size) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    if (hover_anim.find(id) == hover_anim.end()) hover_anim[id] = 0.0f;
    if (click_anim.find(id) == click_anim.end()) click_anim[id] = 0.0f;

    ImVec2 pos = window->DC.CursorPos;

    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    hover_anim[id] = ImLerp(hover_anim[id], hovered ? 1.0f : 0.0f, g.IO.DeltaTime * 10.0f);
    click_anim[id] = ImLerp(click_anim[id], held ? 1.0f : 0.0f, g.IO.DeltaTime * 14.0f);

    float h = hover_anim[id];
    float c = click_anim[id];

    ImDrawList* draw_list = window->DrawList;

    ImU32 col_top = ImGui::GetColorU32(ImLerp(ImVec4(0.10f, 0.10f, 0.12f, 1.0f), ImVec4(0.35f, 0.15f, 0.50f, 1.0f), h));
    ImU32 col_bot = ImGui::GetColorU32(ImLerp(ImVec4(0.07f, 0.07f, 0.09f, 1.0f), ImVec4(0.15f, 0.05f, 0.25f, 1.0f), h));

    ImVec2 visual_min = ImVec2(bb.Min.x + (c * 2), bb.Min.y + (c * 2));
    ImVec2 visual_max = ImVec2(bb.Max.x - (c * 2), bb.Max.y - (c * 2));

    draw_list->AddRectFilledMultiColor(visual_min, visual_max, col_top, col_top, col_bot, col_bot);

    ImU32 border_col = IM_COL32(180, 50, 255, (int)(h * 200));
    draw_list->AddRect(visual_min, visual_max, border_col, 4.0f, 0, 1.5f);


    if (hovered) {
        draw_list->PushClipRect(visual_min, visual_max, true);

        float sweep = sinf((float)ImGui::GetTime() * 1.5f) * 0.5f + 0.5f;
        ImVec2 shine_pos = ImVec2(visual_min.x + (visual_max.x - visual_min.x) * sweep, visual_min.y);

        draw_list->AddRectFilledMultiColor(
            ImVec2(shine_pos.x - 15, visual_min.y), ImVec2(shine_pos.x + 15, visual_max.y),
            IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 30),
            IM_COL32(255, 255, 255, 30), IM_COL32(255, 255, 255, 0)
        );

        draw_list->PopClipRect();
    }


    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImVec2 text_pos = ImVec2(
        visual_min.x + (size.x - text_size.x) * 0.5f,
        visual_min.y + (size.y - text_size.y) * 0.5f
    );

    draw_list->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1), IM_COL32(0, 0, 0, 150), label);
    draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), label);

    return pressed;
}

bool AnimatedButton2(const char* label, ImVec2 size) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    if (hover_anim.find(id) == hover_anim.end()) hover_anim[id] = 0.0f;
    if (click_anim.find(id) == click_anim.end()) click_anim[id] = 0.0f;

    ImVec2 pos = window->DC.CursorPos;

    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    hover_anim[id] = ImLerp(hover_anim[id], hovered ? 1.0f : 0.0f, g.IO.DeltaTime * 10.0f);
    click_anim[id] = ImLerp(click_anim[id], held ? 1.0f : 0.0f, g.IO.DeltaTime * 14.0f);

    float h = hover_anim[id];
    float c = click_anim[id];

    ImDrawList* draw_list = window->DrawList;


    ImU32 col_top = ImGui::GetColorU32(ImLerp(ImVec4(0.10f, 0.10f, 0.12f, 1.0f), ImVec4(0.15f, 0.40f, 0.85f, 1.0f), h));

    ImU32 col_bot = ImGui::GetColorU32(ImLerp(ImVec4(0.07f, 0.07f, 0.09f, 1.0f), ImVec4(0.08f, 0.20f, 0.45f, 1.0f), h));

    ImVec2 visual_min = ImVec2(bb.Min.x + (c * 2), bb.Min.y + (c * 2));
    ImVec2 visual_max = ImVec2(bb.Max.x - (c * 2), bb.Max.y - (c * 2));

    draw_list->AddRectFilledMultiColor(visual_min, visual_max, col_top, col_top, col_bot, col_bot);


    ImU32 border_col = IM_COL32(45, 136, 255, (int)(h * 200));
    draw_list->AddRect(visual_min, visual_max, border_col, 4.0f, 0, 1.5f);


    if (hovered) {
        draw_list->PushClipRect(visual_min, visual_max, true);

        float sweep = sinf((float)ImGui::GetTime() * 1.5f) * 0.5f + 0.5f;
        ImVec2 shine_pos = ImVec2(visual_min.x + (visual_max.x - visual_min.x) * sweep, visual_min.y);

        draw_list->AddRectFilledMultiColor(
            ImVec2(shine_pos.x - 15, visual_min.y), ImVec2(shine_pos.x + 15, visual_max.y),
            IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 30),
            IM_COL32(255, 255, 255, 30), IM_COL32(255, 255, 255, 0)
        );

        draw_list->PopClipRect();
    }

    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImVec2 text_pos = ImVec2(
        visual_min.x + (size.x - text_size.x) * 0.5f,
        visual_min.y + (size.y - text_size.y) * 0.5f
    );

    draw_list->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1), IM_COL32(0, 0, 0, 150), label);
    draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), label);

    return pressed;
}


static std::map<ImGuiID, float> input_hover_anim;
static std::map<ImGuiID, float> input_active_anim;

bool AnimatedTextInput(const char* label, char* buf, size_t buf_size, ImVec2 size, ImGuiInputTextFlags flags = 0) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    if (input_hover_anim.find(id) == input_hover_anim.end()) input_hover_anim[id] = 0.0f;
    if (input_active_anim.find(id) == input_active_anim.end()) input_active_anim[id] = 0.0f;

    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    float h = input_hover_anim[id];
    float a = input_active_anim[id];

    ImDrawList* draw_list = window->DrawList;


    ImU32 bg_col = ImGui::GetColorU32(ImLerp(
        ImLerp(ImVec4(0.06f, 0.06f, 0.08f, 1.0f), ImVec4(0.09f, 0.09f, 0.11f, 1.0f), h),
        ImVec4(0.10f, 0.08f, 0.14f, 1.0f), a));


    draw_list->AddRectFilled(bb.Min, bb.Max, bg_col, 4.0f);


    draw_list->AddRect(bb.Min, bb.Max, IM_COL32(40, 40, 50, 255), 4.0f);


    if (a > 0.01f) {
        float center_x = bb.Min.x + (size.x * 0.5f);
        float line_width = (size.x * 0.5f) * a; 

        ImVec2 line_left(center_x - line_width, bb.Max.y);
        ImVec2 line_right(center_x + line_width, bb.Max.y);


        draw_list->AddLine(line_left, line_right, IM_COL32(160, 50, 255, 255), 2.0f);
    }


    ImGui::SetCursorScreenPos(ImVec2(pos.x + 10, pos.y + (size.y - ImGui::GetTextLineHeight()) * 0.5f));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, 0);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, IM_COL32(138, 43, 226, 120));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    ImGui::PushItemWidth(size.x - 20);

    bool value_changed = ImGui::InputText(label, buf, buf_size, flags);

    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();

    ImGui::PopItemWidth();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);


    input_hover_anim[id] = ImLerp(input_hover_anim[id], hovered ? 1.0f : 0.0f, g.IO.DeltaTime * 10.0f);
    input_active_anim[id] = ImLerp(input_active_anim[id], active ? 1.0f : 0.0f, g.IO.DeltaTime * 14.0f);

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + size.y + g.Style.ItemSpacing.y));

    return value_changed;
}

bool AnimatedTextInput2(const char* label, char* buf, size_t buf_size, ImVec2 size, ImGuiInputTextFlags flags = 0) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    if (input_hover_anim.find(id) == input_hover_anim.end()) input_hover_anim[id] = 0.0f;
    if (input_active_anim.find(id) == input_active_anim.end()) input_active_anim[id] = 0.0f;

    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    float h = input_hover_anim[id];
    float a = input_active_anim[id];

    ImDrawList* draw_list = window->DrawList;


    ImU32 bg_col = ImGui::GetColorU32(ImLerp(
        ImLerp(ImVec4(0.06f, 0.06f, 0.08f, 1.0f), ImVec4(0.09f, 0.09f, 0.11f, 1.0f), h),
        ImVec4(0.05f, 0.08f, 0.15f, 1.0f), a));

    draw_list->AddRectFilled(bb.Min, bb.Max, bg_col, 4.0f);

    draw_list->AddRect(bb.Min, bb.Max, IM_COL32(35, 40, 50, 255), 4.0f);

    if (a > 0.01f) {
        float center_x = bb.Min.x + (size.x * 0.5f);
        float line_width = (size.x * 0.5f) * a;

        ImVec2 line_left(center_x - line_width, bb.Max.y);
        ImVec2 line_right(center_x + line_width, bb.Max.y);


        draw_list->AddLine(line_left, line_right, IM_COL32(45, 136, 255, 255), 2.0f);
    }

    ImGui::SetCursorScreenPos(ImVec2(pos.x + 10, pos.y + (size.y - ImGui::GetTextLineHeight()) * 0.5f));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, 0);


    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, IM_COL32(45, 136, 255, 120));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    ImGui::PushItemWidth(size.x - 20);

    bool value_changed = ImGui::InputText(label, buf, buf_size, flags);

    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();

    ImGui::PopItemWidth();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    input_hover_anim[id] = ImLerp(input_hover_anim[id], hovered ? 1.0f : 0.0f, g.IO.DeltaTime * 10.0f);
    input_active_anim[id] = ImLerp(input_active_anim[id], active ? 1.0f : 0.0f, g.IO.DeltaTime * 14.0f);

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + size.y + g.Style.ItemSpacing.y));

    return value_changed;
}


float tab_alpha[3] = { 0.0f, 0.0f, 0.0f };

bool AnimatedTab(const char* label, bool selected, int id, ImVec2 size) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiID im_id = window->GetID(label);

    ImVec2 p_min = window->DC.CursorPos;
    ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);

    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(ImRect(p_min, p_max), im_id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(ImRect(p_min, p_max), im_id, &hovered, &held);


    float target_alpha = selected ? 1.0f : (hovered ? 0.5f : 0.0f);
    tab_alpha[id] = ImLerp(tab_alpha[id], target_alpha, ImGui::GetIO().DeltaTime * 12.0f);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

   
    ImVec4 bg_color_vec = ImLerp(ImVec4(0.08f, 0.08f, 0.10f, 0.0f), ImVec4(0.15f, 0.15f, 0.18f, 0.7f), tab_alpha[id]);
    draw_list->AddRectFilled(p_min, p_max, ImGui::GetColorU32(bg_color_vec), 5.0f);


    if (tab_alpha[id] > 0.01f) {
        draw_list->AddRectFilled(
            ImVec2(p_min.x, p_min.y + 5),
            ImVec2(p_min.x + 3, p_max.y - 5),
            IM_COL32(138, 43, 226, (int)(tab_alpha[id] * 255)), 2.0f
        );
    }


    ImVec2 text_size = ImGui::CalcTextSize(label);
    float text_x = p_min.x + (size.x - text_size.x) * 0.5f;
    float text_y = p_min.y + (size.y - text_size.y) * 0.5f;


    if (held) text_y += 2.0f;

    draw_list->AddText(ImVec2(text_x, text_y), ImGui::GetColorU32(ImVec4(0.9f, 0.9f, 0.9f, 1.0f)), label);

    return pressed;
}

ID3D11ShaderResourceView* g_VanguardLogoSRV = nullptr;
int g_LogoWidth = 0;
int g_LogoHeight = 0;


bool LoadTextureFromMemory(const unsigned char* image_data, int image_size, ID3D11Device* d3dDevice, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height) {
    int image_width = 0;
    int image_height = 0;
    unsigned char* pixel_data = stbi_load_from_memory(image_data, image_size, &image_width, &image_height, NULL, 4);
    if (pixel_data == NULL) return false;

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = pixel_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    d3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    d3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(pixel_data);

    return true;
}

void PingTrackerThread() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    while (true) {
        struct addrinfo hints = { 0 }, * res;
        hints.ai_family = AF_INET; // IPv4


        if (getaddrinfo("vexyl.xyz", NULL, &hints, &res) == 0) {
            IPAddr ipAddress = ((struct sockaddr_in*)res->ai_addr)->sin_addr.S_un.S_addr;
            freeaddrinfo(res);

            // 2. Send the Ping
            HANDLE hIcmpFile = IcmpCreateFile();
            if (hIcmpFile != INVALID_HANDLE_VALUE) {
                char SendData[32] = "ping";
                DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData) + 8;
                LPVOID ReplyBuffer = malloc(ReplySize);

                if (IcmpSendEcho(hIcmpFile, ipAddress, SendData, sizeof(SendData), NULL, ReplyBuffer, ReplySize, 1000) != 0) {
                    g_UplinkPing = ((PICMP_ECHO_REPLY)ReplyBuffer)->RoundTripTime;
                }
                else {
                    g_UplinkPing = 999; 
                }
                free(ReplyBuffer);
                IcmpCloseHandle(hIcmpFile);
            }
        }
        else {
            g_UplinkPing = 999; 
        }


        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

Vexyl::API g_Auth; // Your existing auth global

ImFont* g_HeaderFont = nullptr;
// --- NEW ASYNC GLOBALS ---
std::atomic<bool> g_IsAuthenticating{ false };
std::atomic<bool> g_AuthFinished{ false };
std::atomic<bool> g_AuthSuccess{ false };
std::string g_AuthErrorMsg = "";

const int WINDOW_WIDTH = 500;
const int WINDOW_HEIGHT = 330; 

std::atomic<DWORD> g_VanguardStatus{ 1 };



static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static HWND g_hwnd = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


struct Notification {
    int id;
    std::string text;
    float time_left;
    float max_time;
    float anim_offset_x;
    float current_y;
};

std::vector<Notification> g_Logs;

void Log(const std::string& message) {
    static int log_id_counter = 0; 
    g_Logs.insert(g_Logs.begin(), { log_id_counter++, message, 3.5f, 3.5f, 50.0f, 15.0f });

}

void RenderNotifications() {
    float dt = ImGui::GetIO().DeltaTime;

    for (int i = 0; i < g_Logs.size(); ++i) {
        auto& log = g_Logs[i];


        if (i >= 4 && log.time_left > 0.5f) {
            log.time_left = 0.5f;
        }

        log.time_left -= dt;


        float target_y = 15.0f + (i * 55.0f);


        log.current_y = ImLerp(log.current_y, target_y, dt * 15.0f);

        float alpha = 1.0f;
        if (log.time_left > log.max_time - 0.3f) {
            alpha = (log.max_time - log.time_left) / 0.3f;
            log.anim_offset_x = ImLerp(log.anim_offset_x, 0.0f, dt * 15.0f); // Slide in
        }
        else if (log.time_left < 0.5f) {
            alpha = log.time_left / 0.5f; 
        }
        else {
            log.anim_offset_x = ImLerp(log.anim_offset_x, 0.0f, dt * 15.0f);
        }

        if (log.time_left <= 0.0f) {
            g_Logs.erase(g_Logs.begin() + i);
            i--;
            continue;
        }


        ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH - 230.0f + log.anim_offset_x, log.current_y));
        ImGui::SetNextWindowSize(ImVec2(215, 45));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.10f, alpha * 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.15f, 0.15f, 0.18f, alpha * 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, alpha));


        ImGui::Begin(("##log_" + std::to_string(log.id)).c_str(), nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings);


        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 win_pos = ImGui::GetWindowPos();
        ImVec2 win_size = ImGui::GetWindowSize();
        draw_list->AddRectFilled(
            ImVec2(win_pos.x + win_size.x - 4, win_pos.y),
            ImVec2(win_pos.x + win_size.x, win_pos.y + win_size.y),
            IM_COL32(138, 43, 226, alpha * 255),
            6.0f, ImDrawFlags_RoundCornersRight
        );

        ImGui::SetCursorPos(ImVec2(12, 14));
        ImGui::TextUnformatted(log.text.c_str());
        ImGui::End();

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
    }
}


void RenderNotifications2() {
    float dt = ImGui::GetIO().DeltaTime;

    for (int i = 0; i < g_Logs.size(); ++i) {
        auto& log = g_Logs[i];

        if (i >= 4 && log.time_left > 0.5f) {
            log.time_left = 0.5f;
        }

        log.time_left -= dt;

        float target_y = 15.0f + (i * 55.0f);

        log.current_y = ImLerp(log.current_y, target_y, dt * 15.0f);

        float alpha = 1.0f;
        if (log.time_left > log.max_time - 0.3f) {
            alpha = (log.max_time - log.time_left) / 0.3f;
            log.anim_offset_x = ImLerp(log.anim_offset_x, 0.0f, dt * 15.0f); // Slide in
        }
        else if (log.time_left < 0.5f) {
            alpha = log.time_left / 0.5f;
        }
        else {
            log.anim_offset_x = ImLerp(log.anim_offset_x, 0.0f, dt * 15.0f);
        }

        if (log.time_left <= 0.0f) {
            g_Logs.erase(g_Logs.begin() + i);
            i--;
            continue;
        }

        ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH - 230.0f + log.anim_offset_x, log.current_y));
        ImGui::SetNextWindowSize(ImVec2(215, 45));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);


        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.07f, 0.10f, alpha * 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.08f, 0.12f, 0.18f, alpha * 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, alpha));

        ImGui::Begin(("##log_" + std::to_string(log.id)).c_str(), nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 win_pos = ImGui::GetWindowPos();
        ImVec2 win_size = ImGui::GetWindowSize();


        draw_list->AddRectFilled(
            ImVec2(win_pos.x + win_size.x - 4, win_pos.y),
            ImVec2(win_pos.x + win_size.x, win_pos.y + win_size.y),
            IM_COL32(45, 136, 255, (int)(alpha * 255.0f)),
            6.0f, ImDrawFlags_RoundCornersRight
        );

        ImGui::SetCursorPos(ImVec2(12, 14));
        ImGui::TextUnformatted(log.text.c_str());
        ImGui::End();

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
    }
}

void DrawSpinner(float radius, float thickness, ImU32 color) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    pos.x += radius; pos.y += radius;
    float time = (float)ImGui::GetTime();

    window->DrawList->PathClear();
    int num_segments = 30;
    float start_angle = time * 6.0f;
    float end_angle = start_angle + IM_PI * 1.2f;
    window->DrawList->PathArcTo(pos, radius, start_angle, end_angle, num_segments);
    window->DrawList->PathStroke(color, 0, thickness);

    ImGui::Dummy(ImVec2(radius * 2, radius * 2));
}

void DrawSpinner2(float radius, float thickness, ImU32 color) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 center = ImVec2(pos.x + radius, pos.y + radius);
    float time = (float)ImGui::GetTime();

    ImDrawList* draw_list = window->DrawList;


    ImU32 bg_track_color = IM_COL32(20, 25, 35, 255);
    draw_list->AddCircle(center, radius, bg_track_color, 30, thickness);


    float spin_speed = time * 5.0f;


    float breathing_effect = (sinf(time * 3.0f) * 0.5f + 0.5f);
    float arc_length = (IM_PI * 0.5f) + (breathing_effect * IM_PI);

    float start_angle = spin_speed;
    float end_angle = start_angle + arc_length;


    draw_list->PathClear();
    draw_list->PathArcTo(center, radius, start_angle, end_angle, 30);
    draw_list->PathStroke(color, 0, thickness);


    ImGui::Dummy(ImVec2(radius * 2, radius * 2));
}


void RenderAAAUI() {
    static int auth_state = 0; 
    static float auth_start_time = 0.0f;
    static char license_key[64] = "";

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.04f, 0.05f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.10f, 0.10f, 0.12f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    ImGui::Begin("CM LOADER", nullptr, window_flags);


    if (g_VanguardLogoSRV != nullptr) {
        ImDrawList* window_draw_list = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetWindowPos();


        float time = (float)ImGui::GetTime();

        float pulse_speed = 3.0f; 
        float min_alpha = 0.1f;  
        float max_alpha = 0.22f;  



        float pulse_wave = (sinf(time * pulse_speed) * 0.5f) + 0.5f;


        float current_alpha = min_alpha + ((max_alpha - min_alpha) * pulse_wave);

        ImU32 logo_color = IM_COL32(255, 255, 255, (int)(current_alpha * 255.0f));


        float target_size = min(WINDOW_WIDTH, WINDOW_HEIGHT) * 0.85f;
        float scale = target_size / max(g_LogoWidth, g_LogoHeight);

        float draw_w = g_LogoWidth * scale;
        float draw_h = g_LogoHeight * scale;


        float center_x = p.x + (WINDOW_WIDTH - draw_w) / 2.0f;
        float center_y = p.y + (WINDOW_HEIGHT - draw_h) / 2.0f;


        window_draw_list->AddImage((void*)g_VanguardLogoSRV,
            ImVec2(center_x, center_y),
            ImVec2(center_x + draw_w, center_y + draw_h),
            ImVec2(0, 0), ImVec2(1, 1), logo_color);
    }

    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetWindowPos();

  
    draw_list->PushClipRect(p, ImVec2(p.x + WINDOW_WIDTH, p.y + 3), true);

    
    draw_list->AddRectFilled(p, ImVec2(p.x + WINDOW_WIDTH, p.y + WINDOW_HEIGHT), IM_COL32(138, 43, 226, 255), 10.0f);

   
    draw_list->PopClipRect();

    
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::InvisibleButton("##drag_zone", ImVec2(WINDOW_WIDTH - 50, 25));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ReleaseCapture();
        SendMessage(g_hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }


    


    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH - 40, 15));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));
    if (ImGui::Button("X", ImVec2(25, 25))) PostQuitMessage(0);
    ImGui::PopStyleColor(3);
    ImGui::PushFont(g_HeaderFont);
    ImGui::SetCursorPos(ImVec2(30, 20));
    ImGui::TextColored(ImVec4(0.54f, 0.17f, 0.89f, 1.0f), "CROWN"); ImGui::SameLine(0, 5);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "METHOD");
    ImGui::PopFont();
    ImGui::SetCursorPos(ImVec2(30, 70));

    if (auth_state == 0) {
       
        ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH / 2 - 140, WINDOW_HEIGHT / 2 - 70));
        ImGui::BeginChild("LoginBlock", ImVec2(280, 160), false);

        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "AUTHENTICATION REQUIRED");
        ImGui::SetNextItemWidth(280);


    
        AnimatedTextInput("##license", license_key, IM_ARRAYSIZE(license_key), ImVec2(280, 35), ImGuiInputTextFlags_Password);


        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.15f, 0.80f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.20f, 0.90f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.10f, 0.60f, 1.0f));

        if (AnimatedButton("VERIFY LICENSE", ImVec2(280, 45))) {
            if (strlen(license_key) > 0 && !g_IsAuthenticating) {
                auth_state = 1; 
                g_IsAuthenticating = true;
                g_AuthFinished = false;

     
                std::thread([key = std::string(license_key)]() {
                    if (g_Auth.license(key)) {
                        g_AuthSuccess = true;
                    }
                    else {
                        g_AuthSuccess = false;
                        g_AuthErrorMsg = g_Auth.error();
                    }
                    g_AuthFinished = true; 
                    g_IsAuthenticating = false;
                    }).detach();

            }
            else if (strlen(license_key) == 0) {
                Log("Please enter a key.");
            }
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndChild();
    }
    else if (auth_state == 1) {

        ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH / 2 - 20, WINDOW_HEIGHT / 2 - 30));
        DrawSpinner(20.0f, 3.0f, IM_COL32(138, 43, 226, 255));

        ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH / 2 - 60, WINDOW_HEIGHT / 2 + 20));
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Authenticating...");

 
        if (g_AuthFinished) {
            if (g_AuthSuccess) {
                auth_state = 2; 
                Log("Authenticated successfully.");
            }
            else {
                auth_state = 0; 
                Log(std::string("Auth Error: ") + g_AuthErrorMsg);
            }

            g_AuthFinished = false;
            g_IsAuthenticating = false;
        }
    }
    else if (auth_state == 2) {
        ImGui::BeginChild("MainContent", ImVec2(WINDOW_WIDTH - 60, WINDOW_HEIGHT - 90), false);


        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "SYSTEM DIAGNOSTICS");
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.09f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::BeginChild("StatusCard", ImVec2(0, 110), true);

        ImGui::SetCursorPos(ImVec2(15, 15));
        ImGui::Columns(2, "StatusColumns", false);

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "System:"); ImGui::Spacing(); ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Vanguard State:"); ImGui::Spacing(); ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Uplink Ping:");

        ImGui::NextColumn();

        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "Supported"); ImGui::Spacing(); ImGui::Spacing();

      
        if (g_VanguardStatus == 1) {
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "Active");
        }
        else if (g_VanguardStatus == 2) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "Emulated");
        }
        else if (g_VanguardStatus == 3)
        {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "Bypassed");
        }

        ImGui::Spacing(); ImGui::Spacing();

        int ping = g_UplinkPing.load();

        if (ping == 0) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Connecting...");
        }
        else if (ping < 20) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "%dms (Stable)", ping);
        }
        else if (ping < 200) {
            ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "%dms (High)", ping);   // Yellow
        }
        else {
            ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Offline / Error", ping); // Red
        }

        ImGui::Columns(1);
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::Spacing(); ImGui::Spacing();

      
        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "EMULATOR:");
        ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);




        float spacing = 10.0f;
        float btn_width = (ImGui::GetContentRegionAvail().x - (spacing * 2)) / 3.0f;


        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));
        if (AnimatedButton("Popup Bypass", ImVec2(btn_width, 45))) {
            std::thread(pb260605).detach();


        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, spacing);


        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));
        if (AnimatedButton("EMULATE (1PC/2PC)", ImVec2(btn_width, 45))) {
            Log("Starting emulator...");
            std::thread(m37286326273).detach();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, spacing);


        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.15f, 0.80f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.20f, 0.90f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.10f, 0.60f, 1.0f));
        if (AnimatedButton("VGC BYPASS (1PC)", ImVec2(btn_width, 45))) {
            Log("Starting bypass...");
            std::thread(b220824).detach();
        }
        ImGui::PopStyleColor(3);


        ImGui::PopStyleVar();

        ImGui::EndChild();
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    RenderNotifications();
}


void RenderNVZUI() {
    static int auth_state = 0;
    static float auth_start_time = 0.0f;
    static char license_key[64] = "";

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoSavedSettings;


    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.04f, 0.06f, 0.98f)); // Slight blue tint to black
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.08f, 0.12f, 0.18f, 1.00f));   // Blue-gray border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    ImGui::Begin("NVZ LOADER", nullptr, window_flags);


    /*
    if (g_VanguardLogoSRV != nullptr) {
        ImDrawList* window_draw_list = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetWindowPos();

        float time = (float)ImGui::GetTime();
        float pulse_speed = 3.0f;
        float min_alpha = 0.1f;
        float max_alpha = 0.22f;

        float pulse_wave = (sinf(time * pulse_speed) * 0.5f) + 0.5f;
        float current_alpha = min_alpha + ((max_alpha - min_alpha) * pulse_wave);
        ImU32 logo_color = IM_COL32(255, 255, 255, (int)(current_alpha * 255.0f));

        float target_size = min(WINDOW_WIDTH, WINDOW_HEIGHT) * 0.85f;
        float scale = target_size / max(g_LogoWidth, g_LogoHeight);

        float draw_w = g_LogoWidth * scale;
        float draw_h = g_LogoHeight * scale;

        float center_x = p.x + (WINDOW_WIDTH - draw_w) / 2.0f;
        float center_y = p.y + (WINDOW_HEIGHT - draw_h) / 2.0f;

        window_draw_list->AddImage((void*)g_VanguardLogoSRV,
            ImVec2(center_x, center_y),
            ImVec2(center_x + draw_w, center_y + draw_h),
            ImVec2(0, 0), ImVec2(1, 1), logo_color);
    }
    */
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetWindowPos();

    draw_list->PushClipRect(p, ImVec2(p.x + WINDOW_WIDTH, p.y + 3), true);


    draw_list->AddRectFilled(p, ImVec2(p.x + WINDOW_WIDTH, p.y + WINDOW_HEIGHT), IM_COL32(45, 136, 255, 255), 10.0f); 

    draw_list->PopClipRect();

    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::InvisibleButton("##drag_zone", ImVec2(WINDOW_WIDTH - 50, 25));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ReleaseCapture();
        SendMessage(g_hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }

    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH - 40, 15));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 0.8f)); 
    if (ImGui::Button("X", ImVec2(25, 25))) PostQuitMessage(0);
    ImGui::PopStyleColor(3);

    ImGui::PushFont(g_HeaderFont);
    ImGui::SetCursorPos(ImVec2(30, 20));


    ImGui::TextColored(ImVec4(0.18f, 0.53f, 1.00f, 1.0f), "NVZ"); ImGui::SameLine(0, 5); 
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "LOUNGE");
    ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(30, 70));

    if (auth_state == 0) {
        ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH / 2 - 140, WINDOW_HEIGHT / 2 - 70));
        ImGui::BeginChild("LoginBlock", ImVec2(280, 160), false);

        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "AUTHENTICATION REQUIRED");
        ImGui::SetNextItemWidth(280);

        AnimatedTextInput2("##license", license_key, IM_ARRAYSIZE(license_key), ImVec2(280, 35), ImGuiInputTextFlags_Password);

        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);


        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.40f, 0.85f, 1.0f));       
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.48f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.08f, 0.30f, 0.70f, 1.0f)); 

        if (AnimatedButton2("VERIFY LICENSE", ImVec2(280, 45))) {
            if (strlen(license_key) > 0 && !g_IsAuthenticating) {
                auth_state = 1;
                g_IsAuthenticating = true;
                g_AuthFinished = false;

                std::thread([key = std::string(license_key)]() {
                    if (g_Auth.license(key)) {
                        g_AuthSuccess = true;
                    }
                    else {
                        g_AuthSuccess = false;
                        g_AuthErrorMsg = g_Auth.error();
                    }
                    g_AuthFinished = true;
                    g_IsAuthenticating = false;
                    }).detach();
            }
            else if (strlen(license_key) == 0) {
                Log("Please enter a key.");
            }
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndChild();
    }
    else if (auth_state == 1) {
        ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH / 2 - 20, WINDOW_HEIGHT / 2 - 30));


        DrawSpinner2(20.0f, 3.0f, IM_COL32(45, 136, 255, 255));

        ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH / 2 - 60, WINDOW_HEIGHT / 2 + 20));
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Authenticating...");

        if (g_AuthFinished) {
            if (g_AuthSuccess) {
                auth_state = 2;
                Log("Authenticated successfully.");
            }
            else {
                auth_state = 0;
                Log(std::string("Auth Error: ") + g_AuthErrorMsg);
            }
            g_AuthFinished = false;
            g_IsAuthenticating = false;
        }
    }
    else if (auth_state == 2) {
        ImGui::BeginChild("MainContent", ImVec2(WINDOW_WIDTH - 60, WINDOW_HEIGHT - 90), false);

        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "SYSTEM DIAGNOSTICS");
        ImGui::Spacing();


        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.07f, 0.10f, 1.0f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::BeginChild("StatusCard", ImVec2(0, 110), true);

        ImGui::SetCursorPos(ImVec2(15, 15));
        ImGui::Columns(2, "StatusColumns", false);

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "System:"); ImGui::Spacing(); ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Vanguard State:"); ImGui::Spacing(); ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Uplink Ping:");

        ImGui::NextColumn();

        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "Supported"); ImGui::Spacing(); ImGui::Spacing();

        if (g_VanguardStatus == 1) {
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "Active");
        }
        else if (g_VanguardStatus == 2) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "Emulated");
        }
        else if (g_VanguardStatus == 3) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "Bypassed");
        }

        ImGui::Spacing(); ImGui::Spacing();

        int ping = g_UplinkPing.load();
        if (ping == 0) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Connecting...");
        }
        else if (ping < 20) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "%dms (Stable)", ping);
        }
        else if (ping < 200) {
            ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "%dms (High)", ping);
        }
        else {
            ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Offline / Error", ping);
        }

        ImGui::Columns(1);
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::Spacing(); ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "EMULATOR:");
        ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

        float spacing = 10.0f;
        float btn_width = (ImGui::GetContentRegionAvail().x - (spacing * 2)) / 3.0f;


        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.13f, 0.18f, 1.0f));    
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.19f, 0.26f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.08f, 0.10f, 0.14f, 1.0f));

        if (AnimatedButton2("Popup Bypass", ImVec2(btn_width, 45))) {
            std::thread(pb260605).detach();


        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, spacing);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.13f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.19f, 0.26f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.08f, 0.10f, 0.14f, 1.0f));

        if (AnimatedButton2("EMULATE (1PC/2PC)", ImVec2(btn_width, 45))) {
            Log("Starting emulator...");
            std::thread(m37286326273).detach();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, spacing);


        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.40f, 0.85f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.48f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.08f, 0.30f, 0.70f, 1.0f));

        if (AnimatedButton2("VGC BYPASS (1PC)", ImVec2(btn_width, 45))) {
            Log("Starting bypass...");
            std::thread(b220824).detach();
        }
        ImGui::PopStyleColor(3);

        ImGui::PopStyleVar();
        ImGui::EndChild();
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    RenderNotifications2();
}


int main(int, char**) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"AAA_Loader_Class", nullptr };
    ::RegisterClassExW(&wc);
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    g_hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        wc.lpszClassName, L"Crown Method",
        WS_POPUP,
        (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    g_Auth.init();

    std::thread(PingTrackerThread).detach();
    SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA);
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(g_hwnd, &margins);

    if (!CreateDeviceD3D(g_hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(g_hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;


    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false; 


    io.Fonts->AddFontFromMemoryTTF((void*)custom_font_data, custom_font_size, 16.0f, &font_cfg);


    g_HeaderFont = io.Fonts->AddFontFromMemoryTTF((void*)header_font_data, header_font_size, 24.0f, &font_cfg);


    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);



    LoadTextureFromMemory(vanguard_logo_data, vanguard_logo_size, g_pd3dDevice, &g_VanguardLogoSRV, &g_LogoWidth, &g_LogoHeight);
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        /////////////////////////////////////////////////////////////////
        //// RRBANDS CHANGE HERE
        /////////////////////////////////////////////////////////////////
          RenderAAAUI();
      //  RenderNVZUI();
        ImGui::Render();

        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(g_hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// --- BOILERPLATE FUNCTIONS ---
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
        // ✅ REPLACE your current WM_NCHITTEST with this:
    case WM_NCHITTEST: {
        // 1. Let ImGui process the mouse first to see if it wants to handle it
        LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);

        // 2. Get mouse position relative to our application window
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);

        // 3. Native Drag Zone: Top 30 pixels, but ignore the rightmost 50 pixels (Close Button)
        if (pt.y >= 0 && pt.y < 30 && pt.x < WINDOW_WIDTH - 50) {
            return HTCAPTION; // Tell Windows: "Treat this area like a title bar for dragging"
        }

        // 4. Otherwise, let ImGui handle the clicks normally
        if (hit == HTCLIENT) return HTCLIENT;
        break;
    }
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK) return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}