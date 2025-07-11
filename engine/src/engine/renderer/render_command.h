#pragma once

// clang-format off
namespace my { enum StencilFlags : uint8_t; }
namespace my { struct GpuMesh; }
namespace my { struct GpuTexture; }
// clang-format on

namespace my {

enum class RenderCommandType {
    Draw,
    Compute,
};

struct DrawCommand {
    uint32_t indexCount = 0;
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;

    uint32_t instanceCount = 1;
    uint32_t instanceOffset = 0;

    int batch_idx = -1;

    const GpuMesh* mesh_data = nullptr;
    const GpuTexture* texture = nullptr;

    int bone_idx = -1;
    int mat_idx = -1;

    uint8_t sortKey = 0;
    StencilFlags flags{ 0 };
};

struct ComputeCommand {
    int dispatchSize[3];
};

struct RenderCommand {
    RenderCommandType type;

    union {
        DrawCommand draw;
        ComputeCommand compute;
    };

    static RenderCommand From(const DrawCommand& p_draw) {
        return { RenderCommandType::Draw, { .draw = p_draw } };
    }

    static RenderCommand From(const ComputeCommand& p_compute) {
        return { RenderCommandType::Compute, { .compute = p_compute } };
    }
};

}  // namespace my