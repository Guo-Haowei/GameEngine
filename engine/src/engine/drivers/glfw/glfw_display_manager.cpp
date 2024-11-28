#include "glfw_display_manager.h"

#include <GLFW/glfw3.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include "engine/core/debugger/profiler.h"
#include "engine/core/framework/application.h"
#include "engine/core/framework/common_dvars.h"
#include "engine/core/framework/input_manager.h"
#include "engine/rendering/graphics_dvars.h"

namespace my {

auto GlfwDisplayManager::InitializeWindow(const WindowSpecfication& p_spec) -> Result<void> {
    m_backend = p_spec.backend;

    glfwSetErrorCallback([](int code, const char* desc) { LOG_FATAL("[glfw] error({}): {}", code, desc); });

    glfwInit();

    glfwWindowHint(GLFW_DECORATED, p_spec.decorated);
    // @TODO: resizable
    // @TODO: fullscreen

    switch (m_backend) {
        case Backend::OPENGL:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
            if (DVAR_GET_BOOL(gfx_gpu_validation)) {
                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
            }
            break;
        case Backend::VULKAN:
        case Backend::METAL:
        case Backend::EMPTY:
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            break;
        default:
            return HBN_ERROR(ErrorCode::ERR_INVALID_PARAMETER);
    }

    m_window = glfwCreateWindow(p_spec.width,
                                p_spec.height,
                                p_spec.title.c_str(),
                                nullptr, nullptr);
    DEV_ASSERT(m_window);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowPos(m_window, 30, 30);
    glfwGetFramebufferSize(m_window, &m_frameSize.x, &m_frameSize.y);

    switch (m_backend) {
        case Backend::OPENGL:
            glfwMakeContextCurrent(m_window);
            ImGui_ImplGlfw_InitForOpenGL(m_window, false);
            break;
        case Backend::VULKAN:
            if (!glfwVulkanSupported()) {
                return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "Vulkan not supported");
            }
            ImGui_ImplGlfw_InitForVulkan(m_window, false);
            break;
        case Backend::METAL:
            ImGui_ImplGlfw_InitForOther(m_window, false);
            break;
        default:
            return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "backend '{}' not supported by glfw", ToString(m_backend));
    }

    glfwSetCursorPosCallback(m_window, CursorPosCallback);
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    glfwSetKeyCallback(m_window, KeyCallback);
    glfwSetScrollCallback(m_window, ScrollCallback);
    glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);

    glfwSetWindowFocusCallback(m_window, ImGui_ImplGlfw_WindowFocusCallback);
    glfwSetCursorEnterCallback(m_window, ImGui_ImplGlfw_CursorEnterCallback);
    glfwSetCharCallback(m_window, ImGui_ImplGlfw_CharCallback);

    return Result<void>();
}

void GlfwDisplayManager::Finalize() {
    ImGui_ImplGlfw_Shutdown();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool GlfwDisplayManager::ShouldClose() {
    return glfwWindowShouldClose(m_window);
}

void GlfwDisplayManager::BeginFrame() {
    glfwPollEvents();
    // glfwGetFramebufferSize(m_window, &m_frameSize.x, &m_frameSize.y);
    glfwGetWindowPos(m_window, &m_windowPos.x, &m_windowPos.y);

    ImGui_ImplGlfw_NewFrame();
}

std::tuple<int, int> GlfwDisplayManager::GetWindowSize() { return std::tuple<int, int>(m_frameSize.x, m_frameSize.y); }

std::tuple<int, int> GlfwDisplayManager::GetWindowPos() { return std::tuple<int, int>(m_windowPos.x, m_windowPos.y); }

void GlfwDisplayManager::Present() {
    if (m_backend == Backend::OPENGL) {
        OPTICK_EVENT();

        GLFWwindow* oldContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(oldContext);

        glfwSwapBuffers(m_window);
    }
}

void GlfwDisplayManager::CursorPosCallback(GLFWwindow* p_window, double p_x, double p_y) {
    ImGui_ImplGlfw_CursorPosCallback(p_window, p_x, p_y);
    // if (!ImGui::GetIO().WantCaptureMouse)
    {
        InputManager::GetSingleton().SetCursor(static_cast<float>(p_x), static_cast<float>(p_y));
    }
}

void GlfwDisplayManager::MouseButtonCallback(GLFWwindow* p_window,
                                             int p_button,
                                             int p_action,
                                             int p_mods) {
    ImGui_ImplGlfw_MouseButtonCallback(p_window, p_button, p_action, p_mods);

    // if (!ImGui::GetIO().WantCaptureMouse)
    {
        if (p_action == GLFW_RELEASE) {
            InputManager::GetSingleton().SetButton(p_button, false);
        } else {
            InputManager::GetSingleton().SetButton(p_button, true);
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
            InputManager::GetSingleton().SetKey(key, true);
        } else if (p_action == GLFW_RELEASE) {
            InputManager::GetSingleton().SetKey(key, false);
        }
    }
}

void GlfwDisplayManager::ScrollCallback(GLFWwindow* p_window,
                                        double p_xoffset,
                                        double p_yoffset) {
    ImGui_ImplGlfw_ScrollCallback(p_window, p_xoffset, p_yoffset);

    // if (!ImGui::GetIO().WantCaptureMouse)
    {
        InputManager::GetSingleton().SetWheel(p_xoffset, p_yoffset);
    }
}

void GlfwDisplayManager::FramebufferSizeCallback(GLFWwindow* p_window, int p_width, int p_height) {
    auto window = reinterpret_cast<GlfwDisplayManager*>(glfwGetWindowUserPointer(p_window));

    auto event = std::make_shared<ResizeEvent>(p_width, p_height);
    window->m_frameSize.x = p_width;
    window->m_frameSize.y = p_height;
    window->m_app->GetEventQueue().DispatchEvent(event);
}

void GlfwDisplayManager::InitializeKeyMapping() {
    if (!m_keyMapping.empty()) {
        return;
    }

    m_keyMapping[GLFW_KEY_SPACE] = KeyCode::KEY_SPACE;
    m_keyMapping[GLFW_KEY_APOSTROPHE] = KeyCode::KEY_APOSTROPHE;
    m_keyMapping[GLFW_KEY_COMMA] = KeyCode::KEY_COMMA;
    m_keyMapping[GLFW_KEY_MINUS] = KeyCode::KEY_MINUS;
    m_keyMapping[GLFW_KEY_PERIOD] = KeyCode::KEY_PERIOD;
    m_keyMapping[GLFW_KEY_SLASH] = KeyCode::KEY_SLASH;
    m_keyMapping[GLFW_KEY_0] = KeyCode::KEY_0;
    m_keyMapping[GLFW_KEY_1] = KeyCode::KEY_1;
    m_keyMapping[GLFW_KEY_2] = KeyCode::KEY_2;
    m_keyMapping[GLFW_KEY_3] = KeyCode::KEY_3;
    m_keyMapping[GLFW_KEY_4] = KeyCode::KEY_4;
    m_keyMapping[GLFW_KEY_5] = KeyCode::KEY_5;
    m_keyMapping[GLFW_KEY_6] = KeyCode::KEY_6;
    m_keyMapping[GLFW_KEY_7] = KeyCode::KEY_7;
    m_keyMapping[GLFW_KEY_8] = KeyCode::KEY_8;
    m_keyMapping[GLFW_KEY_9] = KeyCode::KEY_9;
    m_keyMapping[GLFW_KEY_SEMICOLON] = KeyCode::KEY_SEMICOLON;
    m_keyMapping[GLFW_KEY_EQUAL] = KeyCode::KEY_EQUAL;
    m_keyMapping[GLFW_KEY_A] = KeyCode::KEY_A;
    m_keyMapping[GLFW_KEY_B] = KeyCode::KEY_B;
    m_keyMapping[GLFW_KEY_C] = KeyCode::KEY_C;
    m_keyMapping[GLFW_KEY_D] = KeyCode::KEY_D;
    m_keyMapping[GLFW_KEY_E] = KeyCode::KEY_E;
    m_keyMapping[GLFW_KEY_F] = KeyCode::KEY_F;
    m_keyMapping[GLFW_KEY_G] = KeyCode::KEY_G;
    m_keyMapping[GLFW_KEY_H] = KeyCode::KEY_H;
    m_keyMapping[GLFW_KEY_I] = KeyCode::KEY_I;
    m_keyMapping[GLFW_KEY_J] = KeyCode::KEY_J;
    m_keyMapping[GLFW_KEY_K] = KeyCode::KEY_K;
    m_keyMapping[GLFW_KEY_L] = KeyCode::KEY_L;
    m_keyMapping[GLFW_KEY_M] = KeyCode::KEY_M;
    m_keyMapping[GLFW_KEY_N] = KeyCode::KEY_N;
    m_keyMapping[GLFW_KEY_O] = KeyCode::KEY_O;
    m_keyMapping[GLFW_KEY_P] = KeyCode::KEY_P;
    m_keyMapping[GLFW_KEY_Q] = KeyCode::KEY_Q;
    m_keyMapping[GLFW_KEY_R] = KeyCode::KEY_R;
    m_keyMapping[GLFW_KEY_S] = KeyCode::KEY_S;
    m_keyMapping[GLFW_KEY_T] = KeyCode::KEY_T;
    m_keyMapping[GLFW_KEY_U] = KeyCode::KEY_U;
    m_keyMapping[GLFW_KEY_V] = KeyCode::KEY_V;
    m_keyMapping[GLFW_KEY_W] = KeyCode::KEY_W;
    m_keyMapping[GLFW_KEY_X] = KeyCode::KEY_X;
    m_keyMapping[GLFW_KEY_Y] = KeyCode::KEY_Y;
    m_keyMapping[GLFW_KEY_Z] = KeyCode::KEY_Z;

    m_keyMapping[GLFW_KEY_LEFT_BRACKET] = KeyCode::KEY_LEFT_BRACKET;
    m_keyMapping[GLFW_KEY_BACKSLASH] = KeyCode::KEY_BACKSLASH;
    m_keyMapping[GLFW_KEY_RIGHT_BRACKET] = KeyCode::KEY_RIGHT_BRACKET;
    m_keyMapping[GLFW_KEY_GRAVE_ACCENT] = KeyCode::KEY_GRAVE_ACCENT;
    m_keyMapping[GLFW_KEY_WORLD_1] = KeyCode::KEY_WORLD_1;
    m_keyMapping[GLFW_KEY_WORLD_2] = KeyCode::KEY_WORLD_2;
    m_keyMapping[GLFW_KEY_ESCAPE] = KeyCode::KEY_ESCAPE;
    m_keyMapping[GLFW_KEY_ENTER] = KeyCode::KEY_ENTER;
    m_keyMapping[GLFW_KEY_TAB] = KeyCode::KEY_TAB;
    m_keyMapping[GLFW_KEY_BACKSPACE] = KeyCode::KEY_BACKSPACE;
    m_keyMapping[GLFW_KEY_INSERT] = KeyCode::KEY_INSERT;
    m_keyMapping[GLFW_KEY_DELETE] = KeyCode::KEY_DELETE;
    m_keyMapping[GLFW_KEY_RIGHT] = KeyCode::KEY_RIGHT;
    m_keyMapping[GLFW_KEY_LEFT] = KeyCode::KEY_LEFT;
    m_keyMapping[GLFW_KEY_DOWN] = KeyCode::KEY_DOWN;
    m_keyMapping[GLFW_KEY_UP] = KeyCode::KEY_UP;
    m_keyMapping[GLFW_KEY_PAGE_UP] = KeyCode::KEY_PAGE_UP;
    m_keyMapping[GLFW_KEY_PAGE_DOWN] = KeyCode::KEY_PAGE_DOWN;
    m_keyMapping[GLFW_KEY_HOME] = KeyCode::KEY_HOME;
    m_keyMapping[GLFW_KEY_END] = KeyCode::KEY_END;
    m_keyMapping[GLFW_KEY_CAPS_LOCK] = KeyCode::KEY_CAPS_LOCK;
    m_keyMapping[GLFW_KEY_SCROLL_LOCK] = KeyCode::KEY_SCROLL_LOCK;
    m_keyMapping[GLFW_KEY_NUM_LOCK] = KeyCode::KEY_NUM_LOCK;
    m_keyMapping[GLFW_KEY_PRINT_SCREEN] = KeyCode::KEY_PRINT_SCREEN;
    m_keyMapping[GLFW_KEY_PAUSE] = KeyCode::KEY_PAUSE;
    m_keyMapping[GLFW_KEY_F1] = KeyCode::KEY_F1;
    m_keyMapping[GLFW_KEY_F2] = KeyCode::KEY_F2;
    m_keyMapping[GLFW_KEY_F3] = KeyCode::KEY_F3;
    m_keyMapping[GLFW_KEY_F4] = KeyCode::KEY_F4;
    m_keyMapping[GLFW_KEY_F5] = KeyCode::KEY_F5;
    m_keyMapping[GLFW_KEY_F6] = KeyCode::KEY_F6;
    m_keyMapping[GLFW_KEY_F7] = KeyCode::KEY_F7;
    m_keyMapping[GLFW_KEY_F8] = KeyCode::KEY_F8;
    m_keyMapping[GLFW_KEY_F9] = KeyCode::KEY_F9;
    m_keyMapping[GLFW_KEY_F10] = KeyCode::KEY_F10;
    m_keyMapping[GLFW_KEY_F11] = KeyCode::KEY_F11;
    m_keyMapping[GLFW_KEY_F12] = KeyCode::KEY_F12;
    m_keyMapping[GLFW_KEY_F13] = KeyCode::KEY_F13;
    m_keyMapping[GLFW_KEY_F14] = KeyCode::KEY_F14;
    m_keyMapping[GLFW_KEY_F15] = KeyCode::KEY_F15;
    m_keyMapping[GLFW_KEY_F16] = KeyCode::KEY_F16;
    m_keyMapping[GLFW_KEY_F17] = KeyCode::KEY_F17;
    m_keyMapping[GLFW_KEY_F18] = KeyCode::KEY_F18;
    m_keyMapping[GLFW_KEY_F19] = KeyCode::KEY_F19;
    m_keyMapping[GLFW_KEY_F20] = KeyCode::KEY_F20;
    m_keyMapping[GLFW_KEY_F21] = KeyCode::KEY_F21;
    m_keyMapping[GLFW_KEY_F22] = KeyCode::KEY_F22;
    m_keyMapping[GLFW_KEY_F23] = KeyCode::KEY_F23;
    m_keyMapping[GLFW_KEY_F24] = KeyCode::KEY_F24;
    m_keyMapping[GLFW_KEY_F25] = KeyCode::KEY_F25;
    m_keyMapping[GLFW_KEY_KP_0] = KeyCode::KEY_KP_0;
    m_keyMapping[GLFW_KEY_KP_1] = KeyCode::KEY_KP_1;
    m_keyMapping[GLFW_KEY_KP_2] = KeyCode::KEY_KP_2;
    m_keyMapping[GLFW_KEY_KP_3] = KeyCode::KEY_KP_3;
    m_keyMapping[GLFW_KEY_KP_4] = KeyCode::KEY_KP_4;
    m_keyMapping[GLFW_KEY_KP_5] = KeyCode::KEY_KP_5;
    m_keyMapping[GLFW_KEY_KP_6] = KeyCode::KEY_KP_6;
    m_keyMapping[GLFW_KEY_KP_7] = KeyCode::KEY_KP_7;
    m_keyMapping[GLFW_KEY_KP_8] = KeyCode::KEY_KP_8;
    m_keyMapping[GLFW_KEY_KP_9] = KeyCode::KEY_KP_9;
    m_keyMapping[GLFW_KEY_KP_DECIMAL] = KeyCode::KEY_KP_DECIMAL;
    m_keyMapping[GLFW_KEY_KP_DIVIDE] = KeyCode::KEY_KP_DIVIDE;
    m_keyMapping[GLFW_KEY_KP_MULTIPLY] = KeyCode::KEY_KP_MULTIPLY;
    m_keyMapping[GLFW_KEY_KP_SUBTRACT] = KeyCode::KEY_KP_SUBTRACT;
    m_keyMapping[GLFW_KEY_KP_ADD] = KeyCode::KEY_KP_ADD;
    m_keyMapping[GLFW_KEY_KP_ENTER] = KeyCode::KEY_KP_ENTER;
    m_keyMapping[GLFW_KEY_KP_EQUAL] = KeyCode::KEY_KP_EQUAL;
    m_keyMapping[GLFW_KEY_LEFT_SHIFT] = KeyCode::KEY_LEFT_SHIFT;
    m_keyMapping[GLFW_KEY_LEFT_CONTROL] = KeyCode::KEY_LEFT_CONTROL;
    m_keyMapping[GLFW_KEY_LEFT_ALT] = KeyCode::KEY_LEFT_ALT;
    m_keyMapping[GLFW_KEY_LEFT_SUPER] = KeyCode::KEY_LEFT_SUPER;
    m_keyMapping[GLFW_KEY_RIGHT_SHIFT] = KeyCode::KEY_RIGHT_SHIFT;
    m_keyMapping[GLFW_KEY_RIGHT_CONTROL] = KeyCode::KEY_RIGHT_CONTROL;
    m_keyMapping[GLFW_KEY_RIGHT_ALT] = KeyCode::KEY_RIGHT_ALT;
    m_keyMapping[GLFW_KEY_RIGHT_SUPER] = KeyCode::KEY_RIGHT_SUPER;
    m_keyMapping[GLFW_KEY_MENU] = KeyCode::KEY_MENU;
}

}  // namespace my

#include <imgui/backends/imgui_impl_glfw.cpp>
