#include "glfw_display_manager.h"

#include <GLFW/glfw3.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include "core/debugger/profiler.h"
#include "core/framework/application.h"
#include "core/framework/common_dvars.h"
#include "core/input/input.h"
#include "rendering/graphics_dvars.h"

namespace my {

bool GlfwDisplayManager::InitializeWindow() {

    glfwSetErrorCallback([](int code, const char* desc) { LOG_FATAL("[glfw] error({}): {}", code, desc); });

    glfwInit();

    bool frameless = false;
    glfwWindowHint(GLFW_DECORATED, !frameless);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    if (DVAR_GET_BOOL(gfx_gpu_validation)) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
    }

    const GLFWvidmode* vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    const ivec2 resolution = DVAR_GET_IVEC2(window_resolution);
    const ivec2 min_size = ivec2(600, 400);
    const ivec2 max_size = ivec2(vidmode->width, vidmode->height);
    const ivec2 size = glm::clamp(resolution, min_size, max_size);

    m_window = glfwCreateWindow(size.x, size.y, "Editor (OpenGl)", nullptr, nullptr);
    DEV_ASSERT(m_window);

    glfwSetWindowUserPointer(m_window, this);

    glfwSetWindowPos(m_window, 40, 40);

    glfwMakeContextCurrent(m_window);

    glfwGetFramebufferSize(m_window, &m_frameSize.x, &m_frameSize.y);

    ImGui_ImplGlfw_InitForOpenGL(m_window, false);

    glfwSetCursorPosCallback(m_window, CursorPosCallback);
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    glfwSetKeyCallback(m_window, KeyCallback);
    glfwSetScrollCallback(m_window, ScrollCallback);

    glfwSetWindowFocusCallback(m_window, ImGui_ImplGlfw_WindowFocusCallback);
    glfwSetCursorEnterCallback(m_window, ImGui_ImplGlfw_CursorEnterCallback);
    glfwSetCharCallback(m_window, ImGui_ImplGlfw_CharCallback);

    return true;
}

void GlfwDisplayManager::Finalize() {
    ImGui_ImplGlfw_Shutdown();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool GlfwDisplayManager::ShouldClose() {
    return glfwWindowShouldClose(m_window);
}

void GlfwDisplayManager::NewFrame() {
    glfwPollEvents();
    glfwGetFramebufferSize(m_window, &m_frameSize.x, &m_frameSize.y);
    glfwGetWindowPos(m_window, &m_windowPos.x, &m_windowPos.y);

    ImGui_ImplGlfw_NewFrame();
}

std::tuple<int, int> GlfwDisplayManager::GetWindowSize() { return std::tuple<int, int>(m_frameSize.x, m_frameSize.y); }

std::tuple<int, int> GlfwDisplayManager::GetWindowPos() { return std::tuple<int, int>(m_windowPos.x, m_windowPos.y); }

void GlfwDisplayManager::Present() {
    OPTICK_EVENT();

    GLFWwindow* oldContext = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(oldContext);

    glfwSwapBuffers(m_window);
}

void GlfwDisplayManager::CursorPosCallback(GLFWwindow* p_window, double p_x, double p_y) {
    ImGui_ImplGlfw_CursorPosCallback(p_window, p_x, p_y);
    // if (!ImGui::GetIO().WantCaptureMouse)
    {
        input::SetCursor(static_cast<float>(p_x), static_cast<float>(p_y));
    }
}

void GlfwDisplayManager::MouseButtonCallback(GLFWwindow* p_window,
                                             int p_button,
                                             int p_action,
                                             int p_mods) {
    ImGui_ImplGlfw_MouseButtonCallback(p_window, p_button, p_action, p_mods);

    // if (!ImGui::GetIO().WantCaptureMouse)
    {
        if (p_action == GLFW_PRESS) {
            input::SetButton(p_button, true);
        } else if (p_action == GLFW_RELEASE) {
            input::SetButton(p_button, false);
        }
    }
}

void GlfwDisplayManager::KeyCallback(GLFWwindow* p_window,
                                     int p_keycode,
                                     int p_scancode,
                                     int p_action,
                                     int p_mods) {
    ImGui_ImplGlfw_KeyCallback(p_window, p_keycode, p_scancode, p_action, p_mods);

    auto window = reinterpret_cast<GlfwDisplayManager*>(glfwGetWindowUserPointer(p_window));
    auto& keyMapping = window->m_keyMapping;

    // if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        DEV_ASSERT(keyMapping.find(p_keycode) != keyMapping.end());
        KeyCode key = keyMapping[p_keycode];

        if (p_action == GLFW_PRESS) {
            input::SetKey(key, true);
        } else if (p_action == GLFW_RELEASE) {
            input::SetKey(key, false);
        }
    }
}

void GlfwDisplayManager::ScrollCallback(GLFWwindow* p_window,
                                        double p_xoffset,
                                        double p_yoffset) {
    ImGui_ImplGlfw_ScrollCallback(p_window, p_xoffset, p_yoffset);

    // if (!ImGui::GetIO().WantCaptureMouse)
    {
        input::SetWheel(static_cast<float>(p_xoffset), static_cast<float>(p_yoffset));
    }
}

void GlfwDisplayManager::InitializeKeyMapping() {
    if (!m_keyMapping.empty()) {
        return;
    }

    m_keyMapping[GLFW_KEY_SPACE] = KEY_SPACE;
    m_keyMapping[GLFW_KEY_APOSTROPHE] = KEY_APOSTROPHE;
    m_keyMapping[GLFW_KEY_COMMA] = KEY_COMMA;
    m_keyMapping[GLFW_KEY_MINUS] = KEY_MINUS;
    m_keyMapping[GLFW_KEY_PERIOD] = KEY_PERIOD;
    m_keyMapping[GLFW_KEY_SLASH] = KEY_SLASH;
    m_keyMapping[GLFW_KEY_0] = KEY_0;
    m_keyMapping[GLFW_KEY_1] = KEY_1;
    m_keyMapping[GLFW_KEY_2] = KEY_2;
    m_keyMapping[GLFW_KEY_3] = KEY_3;
    m_keyMapping[GLFW_KEY_4] = KEY_4;
    m_keyMapping[GLFW_KEY_5] = KEY_5;
    m_keyMapping[GLFW_KEY_6] = KEY_6;
    m_keyMapping[GLFW_KEY_7] = KEY_7;
    m_keyMapping[GLFW_KEY_8] = KEY_8;
    m_keyMapping[GLFW_KEY_9] = KEY_9;
    m_keyMapping[GLFW_KEY_SEMICOLON] = KEY_SEMICOLON;
    m_keyMapping[GLFW_KEY_EQUAL] = KEY_EQUAL;
    m_keyMapping[GLFW_KEY_A] = KEY_A;
    m_keyMapping[GLFW_KEY_B] = KEY_B;
    m_keyMapping[GLFW_KEY_C] = KEY_C;
    m_keyMapping[GLFW_KEY_D] = KEY_D;
    m_keyMapping[GLFW_KEY_E] = KEY_E;
    m_keyMapping[GLFW_KEY_F] = KEY_F;
    m_keyMapping[GLFW_KEY_G] = KEY_G;
    m_keyMapping[GLFW_KEY_H] = KEY_H;
    m_keyMapping[GLFW_KEY_I] = KEY_I;
    m_keyMapping[GLFW_KEY_J] = KEY_J;
    m_keyMapping[GLFW_KEY_K] = KEY_K;
    m_keyMapping[GLFW_KEY_L] = KEY_L;
    m_keyMapping[GLFW_KEY_M] = KEY_M;
    m_keyMapping[GLFW_KEY_N] = KEY_N;
    m_keyMapping[GLFW_KEY_O] = KEY_O;
    m_keyMapping[GLFW_KEY_P] = KEY_P;
    m_keyMapping[GLFW_KEY_Q] = KEY_Q;
    m_keyMapping[GLFW_KEY_R] = KEY_R;
    m_keyMapping[GLFW_KEY_S] = KEY_S;
    m_keyMapping[GLFW_KEY_T] = KEY_T;
    m_keyMapping[GLFW_KEY_U] = KEY_U;
    m_keyMapping[GLFW_KEY_V] = KEY_V;
    m_keyMapping[GLFW_KEY_W] = KEY_W;
    m_keyMapping[GLFW_KEY_X] = KEY_X;
    m_keyMapping[GLFW_KEY_Y] = KEY_Y;
    m_keyMapping[GLFW_KEY_Z] = KEY_Z;

    m_keyMapping[GLFW_KEY_LEFT_BRACKET] = KEY_LEFT_BRACKET;
    m_keyMapping[GLFW_KEY_BACKSLASH] = KEY_BACKSLASH;
    m_keyMapping[GLFW_KEY_RIGHT_BRACKET] = KEY_RIGHT_BRACKET;
    m_keyMapping[GLFW_KEY_GRAVE_ACCENT] = KEY_GRAVE_ACCENT;
    m_keyMapping[GLFW_KEY_WORLD_1] = KEY_WORLD_1;
    m_keyMapping[GLFW_KEY_WORLD_2] = KEY_WORLD_2;
    m_keyMapping[GLFW_KEY_ESCAPE] = KEY_ESCAPE;
    m_keyMapping[GLFW_KEY_ENTER] = KEY_ENTER;
    m_keyMapping[GLFW_KEY_TAB] = KEY_TAB;
    m_keyMapping[GLFW_KEY_BACKSPACE] = KEY_BACKSPACE;
    m_keyMapping[GLFW_KEY_INSERT] = KEY_INSERT;
    m_keyMapping[GLFW_KEY_DELETE] = KEY_DELETE;
    m_keyMapping[GLFW_KEY_RIGHT] = KEY_RIGHT;
    m_keyMapping[GLFW_KEY_LEFT] = KEY_LEFT;
    m_keyMapping[GLFW_KEY_DOWN] = KEY_DOWN;
    m_keyMapping[GLFW_KEY_UP] = KEY_UP;
    m_keyMapping[GLFW_KEY_PAGE_UP] = KEY_PAGE_UP;
    m_keyMapping[GLFW_KEY_PAGE_DOWN] = KEY_PAGE_DOWN;
    m_keyMapping[GLFW_KEY_HOME] = KEY_HOME;
    m_keyMapping[GLFW_KEY_END] = KEY_END;
    m_keyMapping[GLFW_KEY_CAPS_LOCK] = KEY_CAPS_LOCK;
    m_keyMapping[GLFW_KEY_SCROLL_LOCK] = KEY_SCROLL_LOCK;
    m_keyMapping[GLFW_KEY_NUM_LOCK] = KEY_NUM_LOCK;
    m_keyMapping[GLFW_KEY_PRINT_SCREEN] = KEY_PRINT_SCREEN;
    m_keyMapping[GLFW_KEY_PAUSE] = KEY_PAUSE;
    m_keyMapping[GLFW_KEY_F1] = KEY_F1;
    m_keyMapping[GLFW_KEY_F2] = KEY_F2;
    m_keyMapping[GLFW_KEY_F3] = KEY_F3;
    m_keyMapping[GLFW_KEY_F4] = KEY_F4;
    m_keyMapping[GLFW_KEY_F5] = KEY_F5;
    m_keyMapping[GLFW_KEY_F6] = KEY_F6;
    m_keyMapping[GLFW_KEY_F7] = KEY_F7;
    m_keyMapping[GLFW_KEY_F8] = KEY_F8;
    m_keyMapping[GLFW_KEY_F9] = KEY_F9;
    m_keyMapping[GLFW_KEY_F10] = KEY_F10;
    m_keyMapping[GLFW_KEY_F11] = KEY_F11;
    m_keyMapping[GLFW_KEY_F12] = KEY_F12;
    m_keyMapping[GLFW_KEY_F13] = KEY_F13;
    m_keyMapping[GLFW_KEY_F14] = KEY_F14;
    m_keyMapping[GLFW_KEY_F15] = KEY_F15;
    m_keyMapping[GLFW_KEY_F16] = KEY_F16;
    m_keyMapping[GLFW_KEY_F17] = KEY_F17;
    m_keyMapping[GLFW_KEY_F18] = KEY_F18;
    m_keyMapping[GLFW_KEY_F19] = KEY_F19;
    m_keyMapping[GLFW_KEY_F20] = KEY_F20;
    m_keyMapping[GLFW_KEY_F21] = KEY_F21;
    m_keyMapping[GLFW_KEY_F22] = KEY_F22;
    m_keyMapping[GLFW_KEY_F23] = KEY_F23;
    m_keyMapping[GLFW_KEY_F24] = KEY_F24;
    m_keyMapping[GLFW_KEY_F25] = KEY_F25;
    m_keyMapping[GLFW_KEY_KP_0] = KEY_KP_0;
    m_keyMapping[GLFW_KEY_KP_1] = KEY_KP_1;
    m_keyMapping[GLFW_KEY_KP_2] = KEY_KP_2;
    m_keyMapping[GLFW_KEY_KP_3] = KEY_KP_3;
    m_keyMapping[GLFW_KEY_KP_4] = KEY_KP_4;
    m_keyMapping[GLFW_KEY_KP_5] = KEY_KP_5;
    m_keyMapping[GLFW_KEY_KP_6] = KEY_KP_6;
    m_keyMapping[GLFW_KEY_KP_7] = KEY_KP_7;
    m_keyMapping[GLFW_KEY_KP_8] = KEY_KP_8;
    m_keyMapping[GLFW_KEY_KP_9] = KEY_KP_9;
    m_keyMapping[GLFW_KEY_KP_DECIMAL] = KEY_KP_DECIMAL;
    m_keyMapping[GLFW_KEY_KP_DIVIDE] = KEY_KP_DIVIDE;
    m_keyMapping[GLFW_KEY_KP_MULTIPLY] = KEY_KP_MULTIPLY;
    m_keyMapping[GLFW_KEY_KP_SUBTRACT] = KEY_KP_SUBTRACT;
    m_keyMapping[GLFW_KEY_KP_ADD] = KEY_KP_ADD;
    m_keyMapping[GLFW_KEY_KP_ENTER] = KEY_KP_ENTER;
    m_keyMapping[GLFW_KEY_KP_EQUAL] = KEY_KP_EQUAL;
    m_keyMapping[GLFW_KEY_LEFT_SHIFT] = KEY_LEFT_SHIFT;
    m_keyMapping[GLFW_KEY_LEFT_CONTROL] = KEY_LEFT_CONTROL;
    m_keyMapping[GLFW_KEY_LEFT_ALT] = KEY_LEFT_ALT;
    m_keyMapping[GLFW_KEY_LEFT_SUPER] = KEY_LEFT_SUPER;
    m_keyMapping[GLFW_KEY_RIGHT_SHIFT] = KEY_RIGHT_SHIFT;
    m_keyMapping[GLFW_KEY_RIGHT_CONTROL] = KEY_RIGHT_CONTROL;
    m_keyMapping[GLFW_KEY_RIGHT_ALT] = KEY_RIGHT_ALT;
    m_keyMapping[GLFW_KEY_RIGHT_SUPER] = KEY_RIGHT_SUPER;
    m_keyMapping[GLFW_KEY_MENU] = KEY_MENU;
}

}  // namespace my

#include <imgui/backends/imgui_impl_glfw.cpp>
