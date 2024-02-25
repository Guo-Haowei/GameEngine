#include "win32_display_manager.h"

#include <imgui/backends/imgui_impl_win32.h>

#include "core/framework/application.h"
#include "core/framework/common_dvars.h"
#include "core/input/input.h"

namespace my {

static LRESULT wnd_proc_wrapper(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        Win32DisplayManager* window = static_cast<Win32DisplayManager*>(lpcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }

    Win32DisplayManager* window = reinterpret_cast<Win32DisplayManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!window) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return window->wnd_proc(hwnd, msg, wParam, lParam);
}

bool Win32DisplayManager::initialize() {
    initialize_key_mapping();

    const ivec2 resolution = DVAR_GET_IVEC2(window_resolution);
    const ivec2 min_size = ivec2(600, 400);
    const ivec2 size = glm::max(min_size, resolution);
    const ivec2 position = DVAR_GET_IVEC2(window_position);

    RECT rect = { 0, 0, size.x, size.y };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    m_wnd_class = { sizeof(m_wnd_class),
                    CS_CLASSDC,
                    wnd_proc_wrapper,
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
                             L"Editor (D3d11)",
                             WS_OVERLAPPEDWINDOW,
                             position.x, position.y,
                             rect.right - rect.left, rect.bottom - rect.top,
                             NULL,
                             NULL,
                             m_wnd_class.hInstance,
                             this);

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
    return std::make_tuple(m_frame_size.x, m_frame_size.y);
}

std::tuple<int, int> Win32DisplayManager::get_window_pos() {
    return std::make_tuple(m_window_pos.x, m_window_pos.y);
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
    // @TODO: move
}

void Win32DisplayManager::present() {
}

LRESULT Win32DisplayManager::wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
        return true;
    }

    switch (msg) {
        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            if (wParam != SIZE_MINIMIZED) {
                // @TODO: only dispatch when resize stops
                m_frame_size.x = width;
                m_frame_size.y = height;
                auto event = std::make_shared<ResizeEvent>(width, height);
                m_app->get_event_queue().dispatch_event(event);
            }
            return 0;
        }
        case WM_MOVE: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            m_window_pos.x = x;
            m_window_pos.y = y;
            return 0;
        }
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) {
                return 0;
            }
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        case WM_DPICHANGED:
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) {
                // const int dpi = HIWORD(wParam);
                // printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
                const RECT* suggested_rect = (RECT*)lParam;
                ::SetWindowPos(hwnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
            break;
        case WM_MOUSEWHEEL: {
            const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            float direction = (delta > 0) ? 1.0f : -1.0f;
            input::set_wheel(0.0f, direction);
        } break;
        case WM_LBUTTONDOWN: {
            input::set_button(MOUSE_BUTTON_LEFT, true);
        } break;
        case WM_LBUTTONUP: {
            input::set_button(MOUSE_BUTTON_LEFT, false);
        } break;
        case WM_RBUTTONDOWN: {
            input::set_button(MOUSE_BUTTON_RIGHT, true);
        } break;
        case WM_RBUTTONUP: {
            input::set_button(MOUSE_BUTTON_RIGHT, false);
        } break;
        case WM_MBUTTONDOWN: {
            input::set_button(MOUSE_BUTTON_MIDDLE, true);
        } break;
        case WM_MBUTTONUP: {
            input::set_button(MOUSE_BUTTON_MIDDLE, false);
        } break;
        case WM_MOUSEMOVE: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            input::set_cursor(static_cast<float>(x), static_cast<float>(y));
        } break;
        case WM_KEYDOWN: {
            int key_code = LOWORD(wParam);
            auto it = m_key_mapping.find(key_code);
            if (it != m_key_mapping.end()) {
                input::set_key(it->second, true);
            } else {
                LOG_WARN("key {} not mapped", key_code);
            }
        } break;
        case WM_KEYUP: {
            int key_code = LOWORD(wParam);
            auto it = m_key_mapping.find(key_code);
            if (it != m_key_mapping.end()) {
                input::set_key(it->second, false);
            }
        } break;
        default:
            break;
    }

    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

void Win32DisplayManager::initialize_key_mapping() {
    DEV_ASSERT(m_key_mapping.empty());

    m_key_mapping[VK_SPACE] = KEY_SPACE;
    for (char i = 0; i <= 9; ++i) {
        m_key_mapping['0' + i] = static_cast<KeyCode>(KEY_0 + i);
    }
    for (char i = 0; i < 26; ++i) {
        m_key_mapping['A' + i] = static_cast<KeyCode>(KEY_A + i);
    }

    // @TODO: do the rest
}

}  // namespace my

#include <imgui/backends/imgui_impl_win32.cpp>
