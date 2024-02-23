#include "gl_subpass.h"

#include "rendering/GLPrerequisites.h"

namespace my {

void GLSubpass::set_render_target() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
    return;
}

void GLSubpass::set_render_target_level(int p_level) const {
    CRASH_NOW();
    unused(p_level);
    return;
}

}  // namespace my
