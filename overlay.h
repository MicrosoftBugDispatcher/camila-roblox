#pragma once
#include <Windows.h>
#include <d3d11.h>

namespace overlay {
    bool Create();
    void Cleanup();
    void Render();
    ID3D11Device* GetDevice();
    ID3D11DeviceContext* GetContext();
    inline bool visible = false;
}

extern HWND g_overlay_hwnd;
extern bool g_overlay_done;
