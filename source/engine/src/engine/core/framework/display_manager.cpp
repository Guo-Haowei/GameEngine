#include "display_manager.h"

#include "drivers/empty/empty_display_manager.h"
#include "drivers/glfw/glfw_display_manager.h"
#if USING(PLATFORM_WINDOWS)
#include "drivers/windows/win32_display_manager.h"
#endif
#include "rendering/graphics_dvars.h"

namespace my {

bool DisplayManager::Initialize() {
    InitializeKeyMapping();
    return InitializeWindow();
}

std::shared_ptr<DisplayManager> DisplayManager::Create() {
    const std::string& backend = DVAR_GET_STRING(gfx_backend);
    if (backend == "opengl") {
        return std::make_shared<GlfwDisplayManager>();
    }
#if USING(PLATFORM_WINDOWS)
    return std::make_shared<Win32DisplayManager>();
#elif USING(PLATFORM_APPLE)
    return std::make_shared<EmptyDisplayManager>();
#else
    return std::make_shared<EmptyDisplayManager>();
#endif
}

}  // namespace my
