#pragma once
#include "engine/core/math/angle.h"
#include "engine/core/math/geomath.h"
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/renderer.h"
#include "engine/systems/ecs/entity.h"

// include this file the last
#include "cbuffer.hlsl.h"

namespace my {
class Scene;
class PerspectiveCameraComponent;
}  // namespace my

namespace my::renderer {

struct RenderDataConfig {
    RenderDataConfig(const Scene& p_scene) : scene(p_scene) {}

    const Scene& scene;
    bool isOpengl{ false };
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
    uint32_t flags;
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

struct DrawData {
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
        std::vector<math::Color> colors;
        uint32_t drawCount;
    } drawDebugContext;

    std::vector<ImageDrawContext> drawImageContext;
    uint32_t drawImageOffset;
};

void PrepareRenderData(const PerspectiveCameraComponent& p_camera,
                       const RenderDataConfig& p_config,
                       DrawData& p_out_data);

}  // namespace my::renderer
