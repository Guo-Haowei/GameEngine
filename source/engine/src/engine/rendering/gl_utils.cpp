#include "gl_utils.h"

#include "drivers/opengl/opengl_prerequisites.h"

namespace gl {

uint64_t MakeTextureResident(uint32_t texture) {
    uint64_t ret = glGetTextureHandleARB(texture);
    glMakeTextureHandleResidentARB(ret);
    return ret;
}

}  // namespace gl