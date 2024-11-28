#include "win32_display_manager.h"

#include <imgui/backends/imgui_impl_win32.h>

#include "engine/core/framework/application.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/input_manager.h"

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

auto Win32DisplayManager::InitializeWindow(const CreateInfo& p_info) -> Result<void> {
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

    return Result<void>();
}

void Win32DisplayManager::Finalize() {
    ImGui_ImplWin32_Shutdown();

    ::DestroyWindow(m_hwnd);
    ::UnregisterClassW(m_wndClass.lpszClassName, m_wndClass.hInstance);
}

bool Win32DisplayManager::ShouldClose() {
    if (!m_shouldQuit) {
        return false;
    }
    ::PostQuitMessage(0);
    return true;
}

std::tuple<int, int> Win32DisplayManager::GetWindowSize() {
    return std::make_tuple(m_frameSize.x, m_frameSize.y);
}

std::tuple<int, int> Win32DisplayManager::GetWindowPos() {
    return std::make_tuple(m_windowPos.x, m_windowPos.y);
}

void Win32DisplayManager::BeginFrame() {
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

    InputManager* input_manager = m_app->GetInputManager();

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
            m_shouldQuit = true;
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
            input_manager->SetWheel(0.0f, direction);
        } break;
        case WM_LBUTTONDOWN: {
            input_manager->SetButton(MOUSE_BUTTON_LEFT, true);
        } break;
        case WM_LBUTTONUP: {
            input_manager->SetButton(MOUSE_BUTTON_LEFT, false);
        } break;
        case WM_RBUTTONDOWN: {
            input_manager->SetButton(MOUSE_BUTTON_RIGHT, true);
        } break;
        case WM_RBUTTONUP: {
            input_manager->SetButton(MOUSE_BUTTON_RIGHT, false);
        } break;
        case WM_MBUTTONDOWN: {
            input_manager->SetButton(MOUSE_BUTTON_MIDDLE, true);
        } break;
        case WM_MBUTTONUP: {
            input_manager->SetButton(MOUSE_BUTTON_MIDDLE, false);
        } break;
        case WM_MOUSEMOVE: {
            int x = LOWORD(p_lparam);
            int y = HIWORD(p_lparam);
            input_manager->SetCursor(static_cast<float>(x), static_cast<float>(y));
        } break;
        case WM_KEYDOWN: {
            const int key = LOWORD(p_wparam);
            auto it = m_keyMapping.find(key);
            if (it != m_keyMapping.end()) {
                input_manager->SetKey(it->second, true);
            } else {
                LOG_WARN("key {} not mapped", key);
                GetKeyState(VK_LSHIFT);
            }
        } break;
        case WM_KEYUP: {
            int key = LOWORD(p_wparam);
            auto it = m_keyMapping.find(key);
            if (it != m_keyMapping.end()) {
                input_manager->SetKey(it->second, false);
            }
        } break;
        default:
            break;
    }

    return ::DefWindowProcW(p_hwnd, p_msg, p_wparam, p_lparam);
}

void Win32DisplayManager::InitializeKeyMapping() {
    DEV_ASSERT(m_keyMapping.empty());

    m_keyMapping[VK_SPACE] = KeyCode::KEY_SPACE;
    m_keyMapping[VK_LEFT] = KeyCode::KEY_LEFT;
    m_keyMapping[VK_RIGHT] = KeyCode::KEY_RIGHT;
    m_keyMapping[VK_UP] = KeyCode::KEY_UP;
    m_keyMapping[VK_DOWN] = KeyCode::KEY_DOWN;
    m_keyMapping[VK_LSHIFT] = KeyCode::KEY_LEFT_SHIFT;
    m_keyMapping[VK_LCONTROL] = KeyCode::KEY_LEFT_CONTROL;
    m_keyMapping[VK_RSHIFT] = KeyCode::KEY_RIGHT_SHIFT;
    m_keyMapping[VK_RCONTROL] = KeyCode::KEY_RIGHT_CONTROL;
    m_keyMapping[VK_SHIFT] = KeyCode::KEY_LEFT_SHIFT;
    m_keyMapping[VK_CONTROL] = KeyCode::KEY_LEFT_CONTROL;

    //    #define VK_LSHIFT
    // #define VK_RSHIFT   0xA1
    // #define VK_LCONTROL 0xA2
    // #define VK_RCONTROL 0xA3

    for (char i = 0; i <= 9; ++i) {
        m_keyMapping['0' + i] = static_cast<KeyCode>((uint16_t)KeyCode::KEY_0 + i);
    }
    for (char i = 0; i < 26; ++i) {
        m_keyMapping['A' + i] = static_cast<KeyCode>((uint16_t)KeyCode::KEY_A + i);
    }

    // @TODO: do the rest
}

}  // namespace my

#include <imgui/backends/imgui_impl_win32.cpp>
