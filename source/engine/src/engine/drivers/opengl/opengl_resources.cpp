#include "opengl_resources.h"

#include "opengl_prerequisites.h"

namespace my {

void OpenGLTexture::Clear() {
    if (handle) {
        // glMakeTextureHandleNonResidentARB();

        glDeleteTextures(1, &handle);
        handle = 0;
        residentHandle = 0;
    }
}
void OpenGLSubpass::Clear() {
    if (handle) {
        glDeleteFramebuffers(1, &handle);
        handle = 0;
    }
}

void OpenGLUniformBuffer::Clear() {
    if (handle) {
        glDeleteBuffers(1, &handle);
        handle = 0;
    }
}

void OpenGLStructuredBuffer::Clear() {
    if (handle) {
        glDeleteBuffers(1, &handle);
        handle = 0;
    }
}

}  // namespace my
