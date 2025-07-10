#pragma once
#include "engine/ecs/entity.h"
#include "engine/math/angle.h"
#include "engine/math/geomath.h"
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/graphics_defines.h"
#include "engine/renderer/renderer.h"
#include "engine/scene/scene_component.h"
#include "render_command.h"

namespace my {
#include "cbuffer.hlsl.h"
}  // namespace my

namespace my {

class Scene;
class CameraComponent;
class RenderPass;
class RenderGraph;

struct RenderOptions {
    bool isOpengl{ false };
    bool ssaoEnabled{ false };
    bool vxgiEnabled{ false };
    bool bloomEnabled{ false };
    bool iblEnabled{ false };
    int debugVoxelId{ 0 };
    int debugBvhDepth{ -1 };
    int voxelTextureSize{ 0 };
    float ssaoKernelRadius{ 0.0f };
};

struct PassContext {
    int pass_idx{ 0 };
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

struct FrameData {
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

    FrameData(const RenderOptions& p_options);

    const RenderOptions options;

    Camera mainCamera;

    // @TODO: multi camera & viewport

    PerFrameConstantBuffer perFrameCache;
    BufferCache<PerBatchConstantBuffer> batchCache;
    BufferCache<MaterialConstantBuffer> materialCache;
    std::vector<PerPassConstantBuffer> passCache;
    std::array<PointShadowConstantBuffer, MAX_POINT_LIGHT_SHADOW_COUNT * 6> pointShadowCache;
    BufferCache<BoneConstantBuffer> boneCache;
    // std::vector<EmitterConstantBuffer> emitterCache;

    // @TODO: rename
    std::array<std::unique_ptr<PassContext>, MAX_POINT_LIGHT_SHADOW_COUNT> pointShadowPasses;
    std::array<PassContext, 1> shadowPasses;  // @TODO: support multi ortho light

    PassContext voxelPass;
    PassContext mainPass;

    // std::vector<InstanceContext> instances;

    // std::vector<ParticleEmitterComponent> emitters;

    // @TODO: refactor
    bool bakeIbl;

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

void PrepareRenderData(const CameraComponent& p_camera,
                       const Scene& p_config,
                       FrameData& p_out_data);

}  // namespace my
