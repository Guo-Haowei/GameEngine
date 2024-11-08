#pragma once
#include "core/framework/display_manager.h"
#include "core/input/input_code.h"
#include "drivers/windows/win32_prerequisites.h"

namespace my {

class Win32DisplayManager : public DisplayManager {
public:
    void Finalize() final;

    bool ShouldClose() final;

    std::tuple<int, int> GetWindowSize() final;
    std::tuple<int, int> GetWindowPos() final;

    void NewFrame() final;
    void Present() final;

    LRESULT WndProc(HWND p_hwnd, UINT p_msg, WPARAM p_wparam, LPARAM p_lparam);

    HWND GetHwnd() const { return m_hwnd; }

private:
    bool InitializeWindow() final;
    void InitializeKeyMapping() final;

    WNDCLASSEXW m_wndClass{};
    HWND m_hwnd{};
    bool m_shouldQuit{ false };

    std::unordered_map<int, KeyCode> m_keyMapping;
};

}  // namespace my
