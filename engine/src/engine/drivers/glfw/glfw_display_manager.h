#pragma once
#include "engine/runtime/display_manager.h"

struct GLFWwindow;

namespace my {

enum class Backend : uint8_t;

class GlfwDisplayManager : public DisplayManager {
public:
    void FinalizeImpl() final;

    bool ShouldClose() final;

    std::tuple<int, int> GetWindowSize() final;
    std::tuple<int, int> GetWindowPos() final;

    void BeginFrame() final;

    void* GetNativeWindow() final;
    GLFWwindow* GetGlfwWindow() const { return m_window; }

private:
    auto InitializeWindow(const WindowSpecfication& p_spec) -> Result<void> final;
    void InitializeKeyMapping() final;

    static void CursorPosCallback(GLFWwindow* p_window, double p_x, double p_y);
    static void MouseButtonCallback(GLFWwindow* p_window, int p_button, int p_action, int p_mods);
    static void KeyCallback(GLFWwindow* p_window, int p_keycode, int p_scancode, int p_action, int p_mods);
    static void ScrollCallback(GLFWwindow* p_window, double p_xoffset, double p_yoffset);
    static void WindowSizeCallback(GLFWwindow* p_window, int p_width, int p_height);

    GLFWwindow* m_window{ nullptr };
    Backend m_backend;
};

}  // namespace my
