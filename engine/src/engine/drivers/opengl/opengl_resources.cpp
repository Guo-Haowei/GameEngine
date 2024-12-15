#include "opengl_resources.h"

#include "opengl_prerequisites.h"

namespace my {

void OpenGlBuffer::Clear() {
    if (handle) {
        uint32_t handle32 = static_cast<uint32_t>(handle);
        glDeleteBuffers(1, &handle32);
        handle = 0;
    }
}

void OpenGlGpuTexture::Clear() {
    if (handle) {
        glDeleteTextures(1, &handle);
        handle = 0;
        residentHandle = 0;
    }
}

void OpenGlDrawPass::Clear() {
    if (handle) {
        glDeleteFramebuffers(1, &handle);
        handle = 0;
    }
}

void OpenGlUniformBuffer::Clear() {
    if (handle) {
        glDeleteBuffers(1, &handle);
        handle = 0;
    }
}

void OpenGlStructuredBuffer::Clear() {
    if (handle) {
        glDeleteBuffers(1, &handle);
        handle = 0;
    }
}

}  // namespace my
