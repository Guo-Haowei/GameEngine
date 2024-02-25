#include "win32_display_manager.h"

#include <imgui/backends/imgui_impl_win32.h>

#include "core/framework/common_dvars.h"

namespace my {

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool Win32DisplayManager::initialize() {
    const ivec2 resolution = DVAR_GET_IVEC2(window_resolution);
    const ivec2 min_size = ivec2(600, 400);
    const ivec2 size = glm::max(min_size, resolution);
    const ivec2 position = DVAR_GET_IVEC2(window_position);

    m_wnd_class = { sizeof(m_wnd_class),
                    CS_CLASSDC,
                    WndProc,
                    0L,
                    0L,
                    GetModuleHandle(NULL),
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    L"MyEngine",
                    NULL };
    ::RegisterClassExW(&m_wnd_class);
    m_hwnd = ::CreateWindowW(m_wnd_class.lpszClassName,
                             L"Editor",
                             WS_OVERLAPPEDWINDOW,
                             position.x, position.y,
                             size.x, size.y,
                             NULL,
                             NULL,
                             m_wnd_class.hInstance,
                             NULL);

    // Show the window
    ::ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(m_hwnd);

    ImGui_ImplWin32_Init(m_hwnd);

    return true;
}

void Win32DisplayManager::finalize() {
    ImGui_ImplWin32_Shutdown();

    ::DestroyWindow(m_hwnd);
    ::UnregisterClassW(m_wnd_class.lpszClassName, m_wnd_class.hInstance);
}

bool Win32DisplayManager::should_close() {
    return m_should_quit;
}

std::tuple<int, int> Win32DisplayManager::get_window_size() {
    return std::make_tuple(0, 0);
}

std::tuple<int, int> Win32DisplayManager::get_window_pos() {
    return std::make_tuple(0, 0);
}

void Win32DisplayManager::new_frame() {
    MSG msg{};
    while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
            m_should_quit = true;
        }
    }

    ImGui_ImplWin32_NewFrame();
}

void Win32DisplayManager::present() {
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
    //     return true;
    // }

    switch (msg) {
        case WM_SIZE:
            // if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            //     CleanupRenderTarget();
            //     g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            //     CreateRenderTarget();
            // }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)  // Disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        case WM_DPICHANGED:
            // if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) {
            //     // const int dpi = HIWORD(wParam);
            //     // printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            //     const RECT* suggested_rect = (RECT*)lParam;
            //     ::SetWindowPos(hWnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
            // }
            break;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

}  // namespace my

#include <imgui/backends/imgui_impl_win32.cpp>
