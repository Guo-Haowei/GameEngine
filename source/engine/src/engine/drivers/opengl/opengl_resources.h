#ifdef OPENGL_RESOURCES_INCLUDED
#error DO NOT INCLUDE THIS FILE IN HEADER
#endif
#define OPENGL_RESOURCES_INCLUDED

#include "drivers/opengl/opengl_prerequisites.h"
#include "rendering/gpu_resource.h"
#include "rendering/render_graph/draw_pass.h"
#include "rendering/uniform_buffer.h"

namespace my {

struct OpenGLTexture : public GpuTexture {
    using GpuTexture::GpuTexture;

    ~OpenGLTexture() {
        clear();
    }

    void clear() {
        if (handle) {
            // glMakeTextureHandleNonResidentARB();

            glDeleteTextures(1, &handle);
            handle = 0;
            resident_handle = 0;
        }
    }

    uint64_t GetResidentHandle() const final { return resident_handle; }
    uint64_t GetHandle() const final { return handle; }

    uint32_t handle = 0;
    uint64_t resident_handle = 0;
};

struct OpenGLSubpass : public DrawPass {
    ~OpenGLSubpass() {
        clear();
    }

    void clear() {
        if (handle) {
            glDeleteFramebuffers(1, &handle);
            handle = 0;
        }
    }

    uint32_t handle = 0;
};

struct OpenGLUniformBuffer : public UniformBufferBase {
    using UniformBufferBase::UniformBufferBase;

    ~OpenGLUniformBuffer() {
        clear();
    }

    void clear() {
        if (handle) {
            glDeleteBuffers(1, &handle);
            handle = 0;
        }
    }

    uint32_t handle = 0;
};

}  // namespace my
