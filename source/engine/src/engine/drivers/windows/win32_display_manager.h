#pragma once
#include "core/framework/display_manager.h"
#include "core/input/input_code.h"
#include "drivers/windows/win32_prerequisites.h"

namespace my {

class Win32DisplayManager : public DisplayManager {
public:
    bool Initialize() final;
    void Finalize() final;

    bool ShouldClose() final;

    std::tuple<int, int> GetWindowSize() final;
    std::tuple<int, int> GetWindowPos() final;

    void NewFrame() final;
    void Present() final;

    LRESULT wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND get_hwnd() const { return m_hwnd; }

private:
    void initialize_key_mapping();

    struct {
        int x, y;
    } m_frame_size, m_window_pos;

    WNDCLASSEXW m_wnd_class{};
    HWND m_hwnd{};
    bool m_should_quit{ false };

    std::unordered_map<int, KeyCode> m_key_mapping;
};

}  // namespace my
