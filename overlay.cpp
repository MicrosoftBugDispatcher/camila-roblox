#include "overlay.h"
#include "memory.h"
#include "globals.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "ext/menu/menu.h"
#include "ext/menu/dex_explorer.h"
#include "ext/helpers/notif.h"
#include "cache.h"
#include "features/esp/esp.h"
#include "features/esp_preview/esp_preview.h"
#include "features/memorymeshchams/memorymeshchams.h"
#include "features/mesh_gpu/mesh_gpu.h"
#include "features/blade_ball/blade_ball.h"
#include "features/aimbot/aimbot.h"
#include "features/walkspeed/walkspeed.h"
#include "features/flight/flight.h"
#include "features/rivals_skin_changer/rivals_skin_changer.h"
#include "features/noclip/noclip.h"
#include "features/hitbox_expander/hitbox_expander.h"
#include "features/skybox_changer/skybox_changer.h"
#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>
#include <cstdio>
#include <ctime>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND g_overlay_hwnd = nullptr;
bool g_overlay_done = false;

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static ID3D11Texture2D* g_depthStencilTexture = nullptr;
static ID3D11DepthStencilView* g_depthStencilView = nullptr;
static HWND g_target_hwnd = nullptr;
static RECT g_last_rect = { 0, 0, 0, 0 };
static bool g_rect_valid = false;

static DWORD g_last_fps_time = 0;
static int g_frame_count = 0;

static bool CreateDeviceD3D(HWND hWnd, int w, int h);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();
static void SyncToTarget();
static bool ResizeSwapChain(UINT width, UINT height);

static BOOL CALLBACK FindWindowByPid(HWND hwnd, LPARAM lParam) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != (DWORD)(uintptr_t)lParam) return TRUE;
    if (!IsWindowVisible(hwnd)) return TRUE;
    WCHAR title[128] = {};
    GetWindowTextW(hwnd, title, 128);
    for (int i = 0; title[i]; i++) if (title[i] >= L'A' && title[i] <= L'Z') title[i] += 32;
    if (wcsstr(title, L"roblox")) {
        g_target_hwnd = hwnd;
        return FALSE;
    }
    return TRUE;}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && g_pd3dDeviceContext && g_pSwapChain) {
            ResizeSwapChain((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        g_overlay_done = true;
        PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

static bool CreateDeviceD3D(HWND hWnd, int w, int h) {
    if (w <= 0 || h <= 0) return false;
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = (UINT)w;
    sd.BufferDesc.Height = (UINT)h;
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

    UINT createFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags, levels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (hr != S_OK) return false;
    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (g_pSwapChain && g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)) == S_OK) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        D3D11_TEXTURE2D_DESC backbuffer_desc{};
        pBackBuffer->GetDesc(&backbuffer_desc);
        D3D11_TEXTURE2D_DESC depth_desc{};
        depth_desc.Width = backbuffer_desc.Width;
        depth_desc.Height = backbuffer_desc.Height;
        depth_desc.MipLevels = 1;
        depth_desc.ArraySize = 1;
        depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depth_desc.SampleDesc = backbuffer_desc.SampleDesc;
        depth_desc.Usage = D3D11_USAGE_DEFAULT;
        depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        if (SUCCEEDED(g_pd3dDevice->CreateTexture2D(&depth_desc, nullptr, &g_depthStencilTexture))) {
            g_pd3dDevice->CreateDepthStencilView(g_depthStencilTexture, nullptr, &g_depthStencilView);
        }
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget() {
    if (g_depthStencilView) { g_depthStencilView->Release(); g_depthStencilView = nullptr; }
    if (g_depthStencilTexture) { g_depthStencilTexture->Release(); g_depthStencilTexture = nullptr; }
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

static bool ResizeSwapChain(UINT width, UINT height) {
    if (!g_pSwapChain || width == 0 || height == 0) return false;
    CleanupRenderTarget();
    HRESULT hr = g_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        std::printf("overlay resize failed: 0x%08X\n", (unsigned int)hr);
        return false;
    }
    CreateRenderTarget();
    return g_mainRenderTargetView != nullptr;
}

static void SyncToTarget() {
    if (!g_target_hwnd || !IsWindow(g_target_hwnd) || !g_overlay_hwnd) return;

    RECT wr = {};
    if (!GetWindowRect(g_target_hwnd, &wr)) return;
    int w = wr.right - wr.left;
    int h = wr.bottom - wr.top;
    if (w <= 0 || h <= 0) return;

    RECT new_rect = { wr.left, wr.top, wr.left + w, wr.top + h };
    if (!g_rect_valid || g_last_rect.left != new_rect.left || g_last_rect.top != new_rect.top || g_last_rect.right != new_rect.right || g_last_rect.bottom != new_rect.bottom) {
        g_last_rect = new_rect;
        g_rect_valid = true;
        SetWindowPos(g_overlay_hwnd, HWND_TOPMOST, wr.left, wr.top, w, h, SWP_NOACTIVATE);
        if (g_pSwapChain && !ResizeSwapChain((UINT)w, (UINT)h)) {
            g_rect_valid = false;
        }
    }
}

bool overlay::Create() {
    g_target_hwnd = nullptr;
    EnumWindows(FindWindowByPid, (LPARAM)(uintptr_t)mem::process_id.load());
    if (!g_target_hwnd) {
        g_target_hwnd = FindWindowW(nullptr, L"Roblox");
        if (!g_target_hwnd) g_target_hwnd = FindWindowW(L"Roblox", nullptr);
    }
    if (!g_target_hwnd) return false;

    RECT tr = {};
    if (!GetWindowRect(g_target_hwnd, &tr)) return false;
    int tw = tr.right - tr.left;
    int th = tr.bottom - tr.top;
    if (tw <= 0 || th <= 0) return false;

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_HREDRAW | CS_VREDRAW, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"CamilaOverlay", nullptr };
    if (!::RegisterClassExW(&wc)) return false;

    g_overlay_hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"", WS_POPUP,
        tr.left, tr.top, tw, th,
        nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_overlay_hwnd) {
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    SetLayeredWindowAttributes(g_overlay_hwnd, 0, 255, LWA_ALPHA);
    MARGINS m = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(g_overlay_hwnd, &m);

    if (!CreateDeviceD3D(g_overlay_hwnd, tw, th)) {
        CleanupDeviceD3D();
        DestroyWindow(g_overlay_hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    ShowWindow(g_overlay_hwnd, SW_SHOWNA);
    g_last_rect = tr;
    g_rect_valid = true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    io.Fonts->AddFontDefault();
    ImFontConfig cfg;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    cfg.PixelSnapH = true;
    if (GetFileAttributesA("C:/font.ttf") != INVALID_FILE_ATTRIBUTES) {
        io.Fonts->AddFontFromFileTTF("C:/font.ttf", 13.0f, &cfg, io.Fonts->GetGlyphRangesDefault());
    }
    io.Fonts->Build();

    if (!ImGui_ImplWin32_Init(g_overlay_hwnd)) return false;
    if (!ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext)) {
        ImGui_ImplWin32_Shutdown();
        return false;
    }
    return true;
}

void overlay::Cleanup() {
    features::ShutdownMeshChams();
    features::ShutdownMemoryMeshChams();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    if (g_overlay_hwnd) {
        DestroyWindow(g_overlay_hwnd);
        g_overlay_hwnd = nullptr;
    }
    UnregisterClassW(L"CamilaOverlay", GetModuleHandle(nullptr));
    g_target_hwnd = nullptr;
    g_rect_valid = false;
}

ID3D11Device* overlay::GetDevice() {
    return g_pd3dDevice;
}

ID3D11DeviceContext* overlay::GetContext() {
    return g_pd3dDeviceContext;
}

void overlay::Render() {
    SyncToTarget();
    if (!g_pd3dDeviceContext || !g_mainRenderTargetView || !g_pSwapChain) return;

    static bool last_visible = false;
    if (overlay::visible != last_visible) {
        LONG ex = GetWindowLongW(g_overlay_hwnd, GWL_EXSTYLE);
        if (overlay::visible)
            ex &= ~WS_EX_TRANSPARENT;
        else
            ex |= WS_EX_TRANSPARENT;
        SetWindowLongW(g_overlay_hwnd, GWL_EXSTYLE, ex);
        last_visible = overlay::visible;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DWORD now_fps = GetTickCount();
    g_frame_count += 1;
    if (g_last_fps_time == 0) {
        g_last_fps_time = now_fps;
    } else {
        DWORD delta = now_fps - g_last_fps_time;
        if (delta >= 1000) {
            fps = (float)g_frame_count * 1000.0f / (float)delta;
            g_frame_count = 0;
            g_last_fps_time = now_fps;
        }
    }

    const float clear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, g_depthStencilView);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear);
    if (g_depthStencilView) {
        g_pd3dDeviceContext->ClearDepthStencilView(g_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    features::RunAimbot();
    features::RunBladeBallAutoParry();

    if (box_esp || healthbar || name || distance) {
        features::RenderESP();
    }
    if (render_expanded_hitbox) {
        features::RenderExpandedHitbox();
    }
    if (skeleton) {
        features::RenderSkeletonESP();
    }
    if (aimviewer) {
        features::RenderAimViewer();
    }
    if (chinahat) {
        features::RenderChinaHatESP();
    }
    if (chams) {
        features::RenderChams();
    }
    if (mesh_chams) {
        features::RenderMeshChams();
    }
    if (memory_mesh_chams) {
        features::RenderMemoryMeshChams();
    }
    features::RenderBladeBallESP();

    features::RenderFOV();
    features::RunWalkspeed();
    features::RunFlight();
    features::RunRivalsSkinChanger();
    features::RunNoclip();
    features::RunHitboxExpander();
    features::RunSkyboxChanger();

    if (show_watermark) {
        static char date_buf[32] = {};
        static char time_buf[32] = {};
        static DWORD last_time_update = 0;
        DWORD now_ms = GetTickCount();
        if (now_ms - last_time_update >= 500) {
            last_time_update = now_ms;
            std::time_t t = std::time(nullptr);
            std::tm lt {};
            localtime_s(&lt, &t);
            std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &lt);
            std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &lt);
        }
        float pad = 8.f;
        float line_h = ImGui::GetTextLineHeight() + 2.f;
        float y = pad;
        ImDrawList* wm_dl = ImGui::GetBackgroundDrawList();
        if (wm_dl) {
            ImU32 text_c = IM_COL32(255, 255, 255, 255);
            wm_dl->AddText(ImVec2(pad, y), text_c, "Camila");
            y += line_h;
            wm_dl->AddText(ImVec2(pad, y), text_c, date_buf);
            y += line_h;
            wm_dl->AddText(ImVec2(pad, y), text_c, time_buf);
            if (show_fps) {
                y += line_h;
                char fps_buf[32];
                std::snprintf(fps_buf, sizeof(fps_buf), "FPS: %.1f", fps);
                wm_dl->AddText(ImVec2(pad, y), text_c, fps_buf);
            }
        }
    }

    if (overlay::visible) {
        RenderMenu();
        features::RenderESPPreview();
    }

    RenderDexExplorer();

    notify::update();
    notify::render();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    const HRESULT present_hr = g_pSwapChain->Present(vsync ? 1 : 0, 0);
    if (FAILED(present_hr) && present_hr != DXGI_STATUS_OCCLUDED) {
        std::printf("overlay present failed: 0x%08X\n", (unsigned int)present_hr);
        RECT client{};
        if (GetClientRect(g_overlay_hwnd, &client)) {
            const UINT width = (UINT)(client.right - client.left);
            const UINT height = (UINT)(client.bottom - client.top);
            ResizeSwapChain(width, height);
        }
    }
}
