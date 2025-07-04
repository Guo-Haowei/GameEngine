#pragma once

#include "../opengl_common/opengl_common_graphics_manager.h"

struct GLFWwindow;

namespace my {

class OpenGl3ESGraphicsManager : public CommonOpenGlGraphicsManager {
public:
    OpenGl3ESGraphicsManager(): CommonOpenGlGraphicsManager() {}

protected:
    auto InitializeInternal() -> Result<void> final;
};

}  // namespace my
