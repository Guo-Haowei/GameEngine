#include "display_manager.h"

#include "drivers/empty/empty_display_manager.h"
#include "drivers/glfw/glfw_display_manager.h"
#include "drivers/windows/win32_display_manager.h"
#include "rendering/rendering_dvars.h"

namespace my {

std::shared_ptr<DisplayManager> DisplayManager::Create() {
    const std::string& backend = DVAR_GET_STRING(r_backend);

    if (backend == "opengl") {
        return std::make_shared<GlfwDisplayManager>();
    } else if (backend == "d3d11") {
        return std::make_shared<Win32DisplayManager>();
    }

    return std::make_shared<EmptyDisplayManager>();
}

}  // namespace my
