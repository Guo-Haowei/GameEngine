#include "win32_display_manager.h"

#include <imgui/backends/imgui_impl_win32.h>

#include "core/framework/application.h"
#include "core/framework/graphics_manager.h"
#include "core/input/input.h"

namespace my {

static LRESULT WndProcWrapper(HWND p_hwnd, UINT p_msg, WPARAM p_wparam, LPARAM p_lparam) {
    if (p_msg == WM_NCCREATE) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(p_lparam);
        Win32DisplayManager* window = static_cast<Win32DisplayManager*>(lpcs->lpCreateParams);
        SetWindowLongPtr(p_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }

    Win32DisplayManager* window = reinterpret_cast<Win32DisplayManager*>(GetWindowLongPtr(p_hwnd, GWLP_USERDATA));
    if (!window) {
        return DefWindowProc(p_hwnd, p_msg, p_wparam, p_lparam);
    }

    return window->WndProc(p_hwnd, p_msg, p_wparam, p_lparam);
}

bool Win32DisplayManager::InitializeWindow(const CreateInfo& p_info) {
    RECT rect = { 0, 0, p_info.width, p_info.height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    std::wstring title(p_info.title.begin(), p_info.title.end());

    m_wndClass = { sizeof(m_wndClass),
                   CS_CLASSDC,
                   WndProcWrapper,
                   0L,
                   0L,
                   GetModuleHandle(NULL),
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   L"MyEngine",
                   NULL };
    ::RegisterClassExW(&m_wndClass);
    m_hwnd = ::CreateWindowW(m_wndClass.lpszClassName,
                             title.c_str(),
                             WS_OVERLAPPEDWINDOW,
                             40, 40,
                             rect.right - rect.left, rect.bottom - rect.top,
                             NULL,
                             NULL,
                             m_wndClass.hInstance,
                             this);

    // Show the window
    ::ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(m_hwnd);

    ImGui_ImplWin32_Init(m_hwnd);

    return true;
}

void Win32DisplayManager::Finalize() {
    ImGui_ImplWin32_Shutdown();

    ::DestroyWindow(m_hwnd);
    ::UnregisterClassW(m_wndClass.lpszClassName, m_wndClass.hInstance);
}

bool Win32DisplayManager::ShouldClose() {
    return m_shouldQuit;
}

std::tuple<int, int> Win32DisplayManager::GetWindowSize() {
    return std::make_tuple(m_frameSize.x, m_frameSize.y);
}

std::tuple<int, int> Win32DisplayManager::GetWindowPos() {
    return std::make_tuple(m_windowPos.x, m_windowPos.y);
}

void Win32DisplayManager::NewFrame() {
    MSG msg{};
    while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
            m_shouldQuit = true;
        }
    }

    ImGui_ImplWin32_NewFrame();
    // @TODO: move
}

void Win32DisplayManager::Present() {
}

LRESULT Win32DisplayManager::WndProc(HWND p_hwnd, UINT p_msg, WPARAM p_wparam, LPARAM p_lparam) {
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
    if (ImGui_ImplWin32_WndProcHandler(p_hwnd, p_msg, p_wparam, p_lparam)) {
        return true;
    }

    switch (p_msg) {
        case WM_SIZE: {
            int width = LOWORD(p_lparam);
            int height = HIWORD(p_lparam);
            if (p_wparam != SIZE_MINIMIZED) {
                // @TODO: only dispatch when resize stops
                m_frameSize.x = width;
                m_frameSize.y = height;
                auto event = std::make_shared<ResizeEvent>(width, height);
                m_app->GetEventQueue().DispatchEvent(event);
            }
            return 0;
        }
        case WM_MOVE: {
            int x = LOWORD(p_lparam);
            int y = HIWORD(p_lparam);
            m_windowPos.x = x;
            m_windowPos.y = y;
            return 0;
        }
        case WM_SYSCOMMAND:
            if ((p_wparam & 0xfff0) == SC_KEYMENU) {
                return 0;
            }
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        case WM_DPICHANGED:
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) {
                // const int dpi = HIWORD(p_wparam);
                // printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
                const RECT* suggested_rect = (RECT*)p_lparam;
                ::SetWindowPos(p_hwnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
            break;
        case WM_MOUSEWHEEL: {
            const int delta = GET_WHEEL_DELTA_WPARAM(p_wparam);
            float direction = (delta > 0) ? 1.0f : -1.0f;
            input::SetWheel(0.0f, direction);
        } break;
        case WM_LBUTTONDOWN: {
            input::SetButton(MOUSE_BUTTON_LEFT, true);
        } break;
        case WM_LBUTTONUP: {
            input::SetButton(MOUSE_BUTTON_LEFT, false);
        } break;
        case WM_RBUTTONDOWN: {
            input::SetButton(MOUSE_BUTTON_RIGHT, true);
        } break;
        case WM_RBUTTONUP: {
            input::SetButton(MOUSE_BUTTON_RIGHT, false);
        } break;
        case WM_MBUTTONDOWN: {
            input::SetButton(MOUSE_BUTTON_MIDDLE, true);
        } break;
        case WM_MBUTTONUP: {
            input::SetButton(MOUSE_BUTTON_MIDDLE, false);
        } break;
        case WM_MOUSEMOVE: {
            int x = LOWORD(p_lparam);
            int y = HIWORD(p_lparam);
            input::SetCursor(static_cast<float>(x), static_cast<float>(y));
        } break;
        case WM_KEYDOWN: {
            int key_code = LOWORD(p_wparam);
            auto it = m_keyMapping.find(key_code);
            if (it != m_keyMapping.end()) {
                input::SetKey(it->second, true);
            } else {
                LOG_WARN("key {} not mapped", key_code);
            }
        } break;
        case WM_KEYUP: {
            int key_code = LOWORD(p_wparam);
            auto it = m_keyMapping.find(key_code);
            if (it != m_keyMapping.end()) {
                input::SetKey(it->second, false);
            }
        } break;
        default:
            break;
    }

    return ::DefWindowProcW(p_hwnd, p_msg, p_wparam, p_lparam);
}

void Win32DisplayManager::InitializeKeyMapping() {
    DEV_ASSERT(m_keyMapping.empty());

    m_keyMapping[VK_SPACE] = KEY_SPACE;
    m_keyMapping[VK_LEFT] = KEY_LEFT;
    m_keyMapping[VK_RIGHT] = KEY_RIGHT;
    m_keyMapping[VK_UP] = KEY_UP;
    m_keyMapping[VK_DOWN] = KEY_DOWN;

    for (char i = 0; i <= 9; ++i) {
        m_keyMapping['0' + i] = static_cast<KeyCode>(KEY_0 + i);
    }
    for (char i = 0; i < 26; ++i) {
        m_keyMapping['A' + i] = static_cast<KeyCode>(KEY_A + i);
    }

    // @TODO: do the rest
}

}  // namespace my

#include <imgui/backends/imgui_impl_win32.cpp>
