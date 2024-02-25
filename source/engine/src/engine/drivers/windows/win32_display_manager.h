#pragma once
#include "core/framework/display_manager.h"
#include "core/input/input_code.h"
#include "drivers/windows/win32_prerequisites.h"

namespace my {

class Win32DisplayManager : public DisplayManager {
public:
    bool initialize() final;
    void finalize() final;

    bool should_close() final;

    std::tuple<int, int> get_window_size() final;
    std::tuple<int, int> get_window_pos() final;

    void new_frame() final;
    void present() final;

private:
    // void initialize_key_mapping();

    struct {
        int x, y;
    } m_frame_size, m_window_pos;

    WNDCLASSEXW m_wnd_class{};
    HWND m_hwnd{};
    bool m_should_quit{ false };

    inline static std::unordered_map<int, KeyCode> s_key_mapping;
};

}  // namespace my
