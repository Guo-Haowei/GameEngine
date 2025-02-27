#pragma once
#include "engine/math/angle.h"
#include "engine/math/geomath.h"
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/graphics_defines.h"
#include "engine/renderer/renderer.h"
#include "engine/scene/scene_component.h"
#include "engine/systems/ecs/entity.h"

namespace my {
#include "cbuffer.hlsl.h"
class Scene;
class PerspectiveCameraComponent;
}  // namespace my

namespace my::renderer {

struct RenderOptions {
    bool isOpengl{ false };
    bool ssaoEnabled{ false };
    bool vxgiEnabled{ false };
    bool bloomEnabled{ false };
    bool dynamicSky{ false };
    int debugVoxelId{ 0 };
    int debugBvhDepth{ -1 };
    int voxelTextureSize{ 0 };
    float ssaoKernelRadius{ 0.0f };
};

struct DrawContext {
    uint32_t index_count;
    uint32_t index_offset;
    int material_idx;
};

struct BatchContext {
    int bone_idx;
    int batch_idx;
    const GpuMesh* mesh_data;
    std::vector<DrawContext> subsets;
    StencilFlags flags = STENCIL_FLAG_NONE;
};

// @TODO: unify BatchContext and InstanceContext
struct InstanceContext {
    const GpuMesh* gpuMesh;
    uint32_t indexCount;
    uint32_t indexOffset;
    uint32_t instanceCount;
    int batchIdx;
    int materialIdx;
    int instanceBufferIndex;
};

struct PassContext {
    int pass_idx{ 0 };

    std::vector<BatchContext> opaque;
    std::vector<BatchContext> transparent;
    std::vector<BatchContext> doubleSided;
};

struct ImageDrawContext {
    int mode;
    uint64_t handle;
    Vector2f size;
    Vector2f position;
};

template<typename BUFFER>
struct BufferCache {
    std::vector<BUFFER> buffer;
    std::unordered_map<ecs::Entity, uint32_t> lookup;

    uint32_t FindOrAdd(ecs::Entity p_entity, const BUFFER& p_buffer) {
        auto it = lookup.find(p_entity);
        if (it != lookup.end()) {
            return it->second;
        }

        uint32_t index = static_cast<uint32_t>(buffer.size());
        lookup[p_entity] = index;
        buffer.emplace_back(p_buffer);
        return index;
    }

    void Clear() {
        buffer.clear();
        lookup.clear();
    }
};

struct RenderData {
    struct Camera {
        Matrix4x4f viewMatrix;
        Matrix4x4f projectionMatrixRendering;
        Matrix4x4f projectionMatrixFrustum;
        Vector3f position;
        Vector3f up;
        Vector3f front;
        Vector3f right;
        float sceenWidth;
        float sceenHeight;
        float aspectRatio;
        float zNear;
        float zFar;
        Degree fovy;
    };

    RenderData(const RenderOptions& p_options) : options(p_options) {
    }

    const RenderOptions options;

    Camera mainCamera;

    // @TODO: multi camera & viewport

    PerFrameConstantBuffer perFrameCache;
    BufferCache<PerBatchConstantBuffer> batchCache;
    BufferCache<MaterialConstantBuffer> materialCache;
    std::vector<PerPassConstantBuffer> passCache;
    std::array<PointShadowConstantBuffer, MAX_POINT_LIGHT_SHADOW_COUNT * 6> pointShadowCache;
    BufferCache<BoneConstantBuffer> boneCache;
    std::vector<EmitterConstantBuffer> emitterCache;

    // @TODO: rename
    std::array<std::unique_ptr<PassContext>, MAX_POINT_LIGHT_SHADOW_COUNT> pointShadowPasses;
    std::array<PassContext, 1> shadowPasses;  // @TODO: support multi ortho light

    PassContext voxelPass;
    PassContext mainPass;

    std::vector<InstanceContext> instances;

    std::vector<ParticleEmitterComponent> emitters;

    // @TODO: refactor
    bool bakeIbl;
    std::shared_ptr<GpuTexture> skyboxHdr;

    struct UpdateBuffer {
        std::vector<Vector3f> positions;
        std::vector<Vector3f> normals;
        const void* id;
    };

    std::vector<UpdateBuffer> updateBuffer;

    struct DrawDebugContext {
        std::vector<Vector3f> positions;
        std::vector<Color> colors;
        uint32_t drawCount;
    } drawDebugContext;

    std::vector<ImageDrawContext> drawImageContext;
    uint32_t drawImageOffset;
};

void PrepareRenderData(const PerspectiveCameraComponent& p_camera,
                       const Scene& p_config,
                       RenderData& p_out_data);

}  // namespace my::renderer
