#include "opengles3_graphics_manager.h"

#include <imgui/backends/imgui_impl_opengl3.h>

#include "../opengl_common/opengl_prerequisites.h"
#include "engine/drivers/glfw/glfw_display_manager.h"
#include "engine/runtime/application.h"
#include "engine/runtime/imgui_manager.h"

namespace my {

auto OpenGLES3GraphicsManager::InitializeInternal() -> Result<void> {
    auto display_manager = dynamic_cast<GlfwDisplayManager*>(m_app->GetDisplayServer());
    DEV_ASSERT(display_manager);
    if (!display_manager) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "display manager is nullptr");
    }
    m_window = display_manager->GetGlfwWindow();

    LOG_VERBOSE("[opengl] renderer: {}", (const char*)glGetString(GL_RENDERER));
    LOG_VERBOSE("[opengl] version: {}", (const char*)glGetString(GL_VERSION));

    m_meshes.set_description("GPU-Mesh-Allocator");

    auto imgui = m_app->GetImguiManager();
    if (imgui) {
        imgui->SetRenderCallbacks(
            []() {
                ImGui_ImplOpenGL3_Init();
                ImGui_ImplOpenGL3_CreateDeviceObjects();
            },
            []() {
                ImGui_ImplOpenGL3_Shutdown();
            });
    }

    return Result<void>();
}

}  // namespace my
