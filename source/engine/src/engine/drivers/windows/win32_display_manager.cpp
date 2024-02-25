#include "win32_display_manager.h"

#include <imgui/backends/imgui_impl_win32.h>

#include "core/framework/common_dvars.h"

// @TODO: move away
#include <d3d11.h>
#include <dxgi.h>
#include <imgui/backends/imgui_impl_dx11.h>
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

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
    const ivec2 resolution = DVAR_GET_IVEC2(window_resolution);
    const ivec2 min_size = ivec2(600, 400);
    const ivec2 size = glm::max(min_size, resolution);
    const ivec2 position = DVAR_GET_IVEC2(window_position);

    RECT rect = { 0, 0, size.x, size.y };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    //// Create the window
    // HWND hwnd = CreateWindow(wc.lpszClassName, L"Window Title", WS_OVERLAPPEDWINDOW,
    //                          CW_USEDEFAULT, CW_USEDEFAULT,
    //                          windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
    //                          NULL, NULL, hInstance, NULL);

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

    // TEMP///////////////////////
    // Initialize Direct3D
    if (!CreateDeviceD3D(m_hwnd)) {
        CleanupDeviceD3D();
        return 1;
    }
    bool ok = ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    DEV_ASSERT(ok);
    ImGui_ImplDX11_NewFrame();
    //////////////////////////

    return true;
}

void Win32DisplayManager::finalize() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    // ImGui_ImplWin32_Shutdown();
    // ImGui::DestroyContext();

    CleanupDeviceD3D();

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
    const float clear_color_with_alpha[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    g_pSwapChain->Present(1, 0);  // Present with vsync
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
            m_frame_size.x = width;
            m_frame_size.y = height;
            if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
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
            // if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) {
            //     // const int dpi = HIWORD(wParam);
            //     // printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            //     const RECT* suggested_rect = (RECT*)lParam;
            //     ::SetWindowPos(hWnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
            // }
            break;
        default:
            break;
    }

    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

}  // namespace my

#include <imgui/backends/imgui_impl_win32.cpp>

///// @TODO: move

// Helper functions
bool CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
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
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)  // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    // Query debug interface if available
    ID3D11Debug* debugInterface = nullptr;
    res = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debugInterface));
    if (FAILED(res)) {
        // Debug layer is not available or not supported
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) {
        g_pSwapChain->Release();
        g_pSwapChain = NULL;
    }
    if (g_pd3dDeviceContext) {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = NULL;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = NULL;
    }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = NULL;
    }
}

#include <imgui/backends/imgui_impl_dx11.cpp>
