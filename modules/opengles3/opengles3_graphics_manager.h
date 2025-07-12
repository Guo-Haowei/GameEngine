#pragma once

#include "../opengl_common/opengl_common_graphics_manager.h"

struct GLFWwindow;

namespace my {

class OpenGLES3GraphicsManager : public CommonOpenGLGraphicsManager {
public:
    OpenGLES3GraphicsManager()
        : CommonOpenGLGraphicsManager() {}

protected:
    auto InitializeInternal() -> Result<void> final;
};

}  // namespace my
