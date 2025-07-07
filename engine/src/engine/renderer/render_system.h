#pragma once
#include "engine/math/angle.h"
#include "engine/math/geomath.h"
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/graphics_defines.h"
#include "engine/renderer/renderer.h"
#include "engine/scene/scene_component.h"
#include "engine/systems/ecs/entity.h"
#include "render_command.h"

// clang-format off
namespace my { class Scene; }
namespace my { class PerspectiveCameraComponent; }
namespace my::renderer { class RenderPass; }
namespace my::renderer { class RenderGraph; }
// clang-format on

namespace my {
#include "cbuffer.hlsl.h"
}  // namespace my

namespace my::renderer {

struct RenderOptions {
    bool isOpengl{ false };
    bool ssaoEnabled{ false };
    bool vxgiEnabled{ false };
    bool bloomEnabled{ false };
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

struct RenderSystem {
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

    RenderSystem(const RenderOptions& p_options);

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

    struct DrawDebugContext {
        std::vector<Vector3f> positions;
        std::vector<Color> colors;
        uint32_t drawCount;
    } drawDebugContext;

    std::vector<ImageDrawContext> drawImageContext;
    uint32_t drawImageOffset;

    ///////////////////////

    void FillLightBuffer(const Scene& p_scene);
    void FillVoxelPass(const Scene& p_scene);
    void FillMainPass(const Scene& p_scene);

private:
    using FilterObjectFunc1 = std::function<bool(const ObjectComponent& p_object)>;
    using FilterObjectFunc2 = std::function<bool(const AABB& p_object_aabb)>;

    void FillPass(const Scene& p_scene,
                  PassContext& p_pass,
                  FilterObjectFunc1 p_filter1,
                  FilterObjectFunc2 p_filter2,
                  RenderPass* p_render_pass,
                  bool p_use_material);

    RenderGraph* m_renderGraph = nullptr;
    AABB voxel_gi_bound;
};

void PrepareRenderData(const PerspectiveCameraComponent& p_camera,
                       const Scene& p_config,
                       RenderSystem& p_out_data);

}  // namespace my::renderer
