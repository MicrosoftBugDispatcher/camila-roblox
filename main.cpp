#include <Windows.h>
#include <cstdio>
#include "process.h"
#include "memory.h"
#include "globals.h"
#include "game.h"
#include "overlay.h"
#include "cache.h"
#include "features/assetmesh/asset_mesh.h"
#include "features/memorymeshchams/memorymeshchams.h"
#include "features/mesh_gpu/mesh_gpu.h"
#include "features/rage/rage.h"
#include "hwid.h"
#include "key_system.h"
#include "anti_debug.h"
#include "key_auth_ui.h"

uintptr_t g_base_address = 0;

int main() {
    SetConsoleTitleA("CamilaRoblox - auth");
    setvbuf(stdout, nullptr, _IONBF, 0);
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    AntiDebug::ProtectProcess();

    //if (!AntiDebug::PerformSecurityChecks()) {
    //    KeyAuthUI::ShowSecurityWarning();
    //    return 1;
    //}

    //if (!KeyAuthUI::ShowKeyAuthenticationUI()) {
    //    KeyAuthUI::ShowAuthFailureAndExit();
    //    return 1;
    //}

    printf("loadin\n");

    if (!overlay::Create()) {

        return 1;
    }

    uint32_t pid = 0;
    uintptr_t base = 0;
    if (!process::FindRoblox(pid, base)) {

        overlay::Cleanup();
        return 1;
    }

    mem::process_id.store(pid);
    g_base_address = base;

    assetmesh::initialize();

    if (!mem::grabroblox_h()) {

        overlay::Cleanup();
        return 1;
    }

    uint64_t place_id = game::GetPlaceId(g_base_address);
    current_place_id = place_id;
    printf("placeid: %llu\n", (unsigned long long)place_id);

    features::rage::Initialize();
    features::StartMemoryMeshChams();

    static DWORD last_print = 0;
    static bool rshift_held = false;
    MSG msg = {};

    while (!g_overlay_done) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) g_overlay_done = true;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        __try {
            if (GetAsyncKeyState(VK_RSHIFT) & 0x8000) {
                if (!rshift_held) {
                    overlay::visible = !overlay::visible;
                    rshift_held = true;
                }
            } else {
                overlay::Render();
                rshift_held = false;
            }
            cache::Update(g_base_address);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            const unsigned int code = (unsigned int)GetExceptionCode();
            std::printf("frame exception recovered: 0x%08X\n", code);
            Sleep(25);
        }

        DWORD now = GetTickCount();
        if (now - last_print >= 2000) {
            size_t count = cache::GetEntityCount();
            uint64_t new_place_id = game::GetPlaceId(g_base_address);
            if (new_place_id != current_place_id) {
                current_place_id = new_place_id;
            }
            printf("players: %zu placeid: %llu\n", count, (unsigned long long)current_place_id);
            last_print = now;
        }
        SwitchToThread();
    }

    assetmesh::shutdown();
    features::rage::Shutdown();
    overlay::Cleanup();
    return 0;
}
