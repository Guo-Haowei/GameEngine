#include "display_manager.h"

#include "drivers/glfw/glfw_display_manager.h"
#include "drivers/windows/win32_display_manager.h"
#include "rendering/rendering_dvars.h"

namespace my {

std::shared_ptr<DisplayManager> DisplayManager::create() {
    const std::string& backend = DVAR_GET_STRING(r_backend);

    if (backend == "opengl") {
        return std::make_shared<GlfwDisplayManager>();
    } else if (backend == "d3d11") {
        // @TODO:
        return std::make_shared<Win32DisplayManager>();
    }

    CRASH_NOW_MSG("no suitable display manager");
    return nullptr;
}

}  // namespace my
