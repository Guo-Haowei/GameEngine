#ifdef OPENGL_RESOURCES_INCLUDED
#error DO NOT INCLUDE THIS FILE IN HEADER
#endif
#define OPENGL_RESOURCES_INCLUDED

#include "rendering/gpu_resource.h"
#include "rendering/render_graph/draw_pass.h"
#include "rendering/uniform_buffer.h"

namespace my {

struct OpenGLTexture : public GpuTexture {
    using GpuTexture::GpuTexture;

    ~OpenGLTexture() {
        Clear();
    }

    void Clear();

    uint64_t GetResidentHandle() const final { return residentHandle; }
    uint64_t GetHandle() const final { return handle; }

    uint32_t handle = 0;
    uint64_t residentHandle = 0;
};

struct OpenGLSubpass : public DrawPass {
    ~OpenGLSubpass() {
        Clear();
    }

    void Clear();

    uint32_t handle = 0;
};

struct OpenGLUniformBuffer : public ConstantBufferBase {
    using ConstantBufferBase::ConstantBufferBase;

    ~OpenGLUniformBuffer() {
        Clear();
    }

    void Clear();

    uint32_t handle = 0;
};

struct OpenGLStructuredBuffer : public GpuStructuredBuffer {
    using GpuStructuredBuffer::GpuStructuredBuffer;

    ~OpenGLStructuredBuffer() {
        Clear();
    }

    void Clear();

    uint32_t handle = 0;
};

}  // namespace my
