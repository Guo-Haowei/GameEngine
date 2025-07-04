#pragma once

#include "../opengl_common/opengl_common_graphics_manager.h"

struct GLFWwindow;

namespace my {

class OpenGLES3GraphicsManager : public CommonOpenGlGraphicsManager {
public:
    OpenGLES3GraphicsManager(): CommonOpenGlGraphicsManager() {}

protected:
    auto InitializeInternal() -> Result<void> final;
};

}  // namespace my
