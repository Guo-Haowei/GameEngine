#include "opengl_texture.h"

#include "drivers/opengl/opengl_prerequisites.h"

namespace my {

OpenGLTexture::~OpenGLTexture() {
    if (handle) {
        // glMakeTextureHandleNonResidentARB();

        glDeleteTextures(1, &handle);
        handle = 0;
        resident_handle = 0;
    }
}

}  // namespace my
