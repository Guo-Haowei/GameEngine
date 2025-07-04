#pragma once

#include "../opengl_common/opengl_common_graphics_manager.h"

struct GLFWwindow;

namespace my {

class OpenGl4GraphicsManager : public CommonOpenGlGraphicsManager {
public:
    OpenGl4GraphicsManager(): CommonOpenGlGraphicsManager() {}

protected:
    auto InitializeInternal() -> Result<void> final;
};

}  // namespace my
