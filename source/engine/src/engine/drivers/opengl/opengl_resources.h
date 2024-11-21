#ifdef OPENGL_RESOURCES_INCLUDED
#error DO NOT INCLUDE THIS FILE IN HEADER
#endif
#define OPENGL_RESOURCES_INCLUDED

#include "rendering/gpu_resource.h"
#include "rendering/render_graph/draw_pass.h"

namespace my {

struct OpenGlGpuTexture : public GpuTexture {
    using GpuTexture::GpuTexture;

    ~OpenGlGpuTexture() {
        Clear();
    }

    void Clear();

    uint64_t GetResidentHandle() const final { return residentHandle; }
    uint64_t GetHandle() const final { return handle; }
    uint64_t GetUavHandle() const final { return handle; }

    uint32_t handle = 0;
    uint64_t residentHandle = 0;
};

struct OpenGlDrawPass : public DrawPass {
    ~OpenGlDrawPass() {
        Clear();
    }

    void Clear();

    uint32_t handle = 0;
};

struct OpenGlUniformBuffer : public GpuConstantBuffer {
    using GpuConstantBuffer::GpuConstantBuffer;

    ~OpenGlUniformBuffer() {
        Clear();
    }

    void Clear();

    uint32_t handle = 0;
};

struct OpenGlStructuredBuffer : public GpuStructuredBuffer {
    using GpuStructuredBuffer::GpuStructuredBuffer;

    ~OpenGlStructuredBuffer() {
        Clear();
    }

    void Clear();

    uint32_t handle = 0;
};

}  // namespace my
