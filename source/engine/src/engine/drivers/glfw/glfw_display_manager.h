#pragma once
#include "core/framework/display_manager.h"

struct GLFWwindow;

namespace my {

enum class Backend : uint8_t;

class GlfwDisplayManager : public DisplayManager {
public:
    void Finalize() final;

    bool ShouldClose() final;

    std::tuple<int, int> GetWindowSize() final;
    std::tuple<int, int> GetWindowPos() final;

    void BeginFrame() final;
    void Present() final;

private:
    auto InitializeWindow(const CreateInfo& p_info) -> Result<void> final;
    void InitializeKeyMapping() final;

    static void CursorPosCallback(GLFWwindow* p_window, double p_x, double p_y);
    static void MouseButtonCallback(GLFWwindow* p_window, int p_button, int p_action, int p_mods);
    static void KeyCallback(GLFWwindow* p_window, int p_keycode, int p_scancode, int p_action, int p_mods);
    static void ScrollCallback(GLFWwindow* p_window, double p_xoffset, double p_yoffset);

    GLFWwindow* m_window{ nullptr };
    Backend m_backend;
};

}  // namespace my
