#include "render_system.h"

#include "engine/core/base/random.h"
#include "engine/core/debugger/profiler.h"
#include "engine/math/matrix_transform.h"
#include "engine/render_graph/render_graph_defines.h"
#include "engine/renderer/frame_data.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/path_tracer_render_system.h"
#include "engine/runtime/application.h"
#include "engine/runtime/graphics_manager_interface.h"
#include "engine/scene/scene.h"

namespace my {

extern void RunMeshRenderSystem(Scene& p_scene, FrameData& p_framedata);
extern void RunTileMapRenderSystem(Scene& p_scene, FrameData& p_framedata);

auto RenderSystem::InitializeImpl() -> Result<void> {
    return Result<void>();
}

void RenderSystem::FinalizeImpl() {
}

#if 0
static void DebugDrawBVH(int p_level, BvhAccel* p_bvh, const Matrix4x4f* p_matrix) {
    if (!p_bvh || p_bvh->depth > p_level) {
        return;
    }

    if (p_bvh->depth == p_level) {
        renderer::AddDebugCube(p_bvh->aabb,
                               Color::HexRgba(0xFFFF0037),
                               p_matrix);
    }

    DebugDrawBVH(p_level, p_bvh->left.get(), p_matrix);
    DebugDrawBVH(p_level, p_bvh->right.get(), p_matrix);
};
#endif

using KernelData = std::array<Vector4f, 64>;

static_assert(sizeof(KernelData) == sizeof(Vector4f) * SSAO_KERNEL_SIZE);

static KernelData GenerateSsaoKernel() {
    auto lerp = [](float a, float b, float f) {
        return a + f * (b - a);
    };

    KernelData kernel;

    const int kernel_size = 32;
    const float inv_kernel_size = 1.0f / kernel_size;
    for (int i = 0; i < kernel.size(); ++i) {
        // [-1, 1], [-1, 1], [0, 1]
        Vector3f sample(Random::Float(-1.0f, 1.0f),
                        Random::Float(-1.0f, 1.0f),
                        Random::Float());

        sample = normalize(sample);
        sample *= Random::Float();
        float scale = i * inv_kernel_size;

        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        kernel[i].xyz = sample;
    }

    return kernel;
}

static void FillConstantBuffer(const Scene& p_scene, FrameData& p_out_data) {
    const auto& options = p_out_data.options;
    auto& cache = p_out_data.perFrameCache;

    // camera
    {
        const auto& camera = p_out_data.mainCamera;
        cache.c_camView = camera.viewMatrix;
        cache.c_camProj = camera.projectionMatrixRendering;
        cache.c_invCamView = glm::inverse(camera.viewMatrix);
        cache.c_invCamProj = glm::inverse(camera.projectionMatrixRendering);
        cache.c_cameraFovDegree = camera.fovy.GetDegree();
        cache.c_cameraForward = camera.front;
        cache.c_cameraRight = camera.right;
        cache.c_cameraUp = camera.up;
        cache.c_cameraPosition = camera.position;
    }

    // Bloom
    {
        cache.c_bloomThreshold = 1.3f;
        cache.c_enableBloom = options.bloomEnabled;

        cache.c_debugVoxelId = options.debugVoxelId;
        cache.c_ptObjectCount = (int)p_scene.m_MeshRendererComponents.GetCount();
    }

    // IBL
    {
        cache.c_iblEnabled = options.iblEnabled;
    }

    // SSAO
    {
        // @TODO: do this properly
        static auto kernel_data = GenerateSsaoKernel();
        cache.c_ssaoEnabled = options.ssaoEnabled;
        cache.c_ssaoKernalRadius = options.ssaoKernelRadius;
        constexpr size_t kernel_size = sizeof(kernel_data);
        static_assert(sizeof(cache.c_ssaoKernel) == kernel_size);
        memcpy(cache.c_ssaoKernel, kernel_data.data(), kernel_size);
    }

    // @TODO: refactor
    static int s_frameIndex = 0;
    cache.c_frameIndex = s_frameIndex++;
    // @TODO: fix this
    cache.c_sceneDirty = p_scene.GetDirtyFlags() != SCENE_DIRTY_NONE;

    // Force fields
    int counter = 0;
    for (auto [id, force_field_component] : p_scene.m_ForceFieldComponents) {
        ForceField& force_field = cache.c_forceFields[counter++];
        const TransformComponent& transform = *p_scene.GetComponent<TransformComponent>(id);
        force_field.position = transform.GetTranslation();
        force_field.strength = force_field_component.strength;
    }

    cache.c_forceFieldsCount = counter;

    // @TODO: fix
#if 0
    for (auto const [entity, environment] : p_scene.View<EnvironmentComponent>()) {
        cache.c_ambientColor = environment.ambient.color;
        if (!environment.sky.texturePath.empty()) {
            environment.sky.textureAsset = AssetRegistry::GetSingleton().Request<ImageAsset>(environment.sky.texturePath);
        }
    }

    // @TODO:
    const int level = options.debugBvhDepth;
    if (level > -1) {
        for (auto const [id, obj] : p_scene.m_MeshRendererComponents) {
            const MeshComponent* mesh = p_scene.GetComponent<MeshComponent>(obj.meshId);
            const TransformComponent* transform = p_scene.GetComponent<TransformComponent>(id);
            if (mesh && transform) {
                if (const auto& bvh = mesh->bvh; bvh) {
                    const auto& matrix = transform->GetWorldMatrix();
                    DebugDrawBVH(level, bvh.get(), &matrix);
                }
            }
        }
    }
#endif
}

static void FillEnvConstants(const Scene&,
                             FrameData& p_out_data) {
    // @TODO: return if necessary

    constexpr int count = IBL_MIP_CHAIN_MAX * 6;
    if (p_out_data.batchCache.buffer.size() < count) {
        p_out_data.batchCache.buffer.resize(count);
    }

    auto matrices = p_out_data.options.isOpengl ? BuildOpenGlCubeMapViewProjectionMatrix(Vector3f(0)) : BuildCubeMapViewProjectionMatrix(Vector3f(0));
    for (int mip_idx = 0; mip_idx < IBL_MIP_CHAIN_MAX; ++mip_idx) {
        for (int face_id = 0; face_id < 6; ++face_id) {
            auto& batch = p_out_data.batchCache.buffer[mip_idx * 6 + face_id];
            batch.c_cubeProjectionViewMatrix = matrices[face_id];
            batch.c_envPassRoughness = (float)mip_idx / (float)(IBL_MIP_CHAIN_MAX - 1);
        }
    }
}

void RenderSystem::BeginFrame() {
    if (m_frameData) {
        delete m_frameData;
        m_frameData = nullptr;
    }

    RenderOptions options = {
        .isOpengl = m_app->GetGraphicsManager()->GetBackend() == Backend::OPENGL,
        .ssaoEnabled = DVAR_GET_BOOL(gfx_ssao_enabled),
        .vxgiEnabled = false,
        .bloomEnabled = DVAR_GET_BOOL(gfx_enable_bloom),
        .iblEnabled = DVAR_GET_BOOL(gfx_enable_ibl),
        .debugVoxelId = DVAR_GET_INT(gfx_debug_vxgi_voxel),
        .debugBvhDepth = DVAR_GET_INT(gfx_bvh_debug),
        .voxelTextureSize = DVAR_GET_INT(gfx_voxel_size),
        .ssaoKernelRadius = DVAR_GET_FLOAT(gfx_ssao_radius),
    };

    // @HACK
    m_frameData = new FrameData(options);
    static bool s_firstFrame = true;
    m_frameData->bakeIbl = s_firstFrame;
    s_firstFrame = false;
}

void RenderSystem::RenderFrame(Scene& p_scene) {
    // HACK
    auto backend = m_app->GetGraphicsManager()->GetBackend();
    switch (backend) {
        case my::Backend::OPENGL:
        case my::Backend::D3D11:
        case my::Backend::D3D12:
            break;
        default:
            return;
    }

    HBN_PROFILE_EVENT();
    CameraComponent& camera = *m_app->GetActiveCamera();

    DEV_ASSERT(m_frameData);
    FrameData& framedata = *m_frameData;

    FillCameraData(camera, framedata);
    FillConstantBuffer(p_scene, framedata);

    RunMeshRenderSystem(p_scene, framedata);

    RunTileMapRenderSystem(p_scene, framedata);

    // @TODO: RunSprite
    // @TODO: RunTileMap
#if 0
    FillMeshEmitterBuffer(p_scene, p_out_data);
    FillParticleEmitterBuffer(p_scene, p_out_data);
#endif

    FillEnvConstants(p_scene, framedata);

    RequestPathTracerUpdate(camera, p_scene);

    auto& context = m_frameData->drawDebugContext;
    context.drawCount = (uint32_t)context.positions.size();
}

void RenderSystem::FillCameraData(const CameraComponent& p_camera, FrameData& p_framedata) {

    auto reverse_z = [](Matrix4x4f& p_perspective) {
        constexpr Matrix4x4f matrix{ 1.0f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 1.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, -1.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 1.0f };
        p_perspective = matrix * p_perspective;
    };
    auto normalize_unit_range = [](Matrix4x4f& p_perspective) {
        constexpr Matrix4x4f matrix{ 1.0f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 1.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 0.5f, 0.0f,
                                     0.0f, 0.0f, 0.5f, 1.0f };
        p_perspective = matrix * p_perspective;
    };

    auto& camera = p_framedata.mainCamera;
    camera.sceenWidth = static_cast<float>(p_camera.GetWidth());
    camera.sceenHeight = static_cast<float>(p_camera.GetHeight());
    camera.aspectRatio = camera.sceenWidth / camera.sceenHeight;
    camera.fovy = p_camera.GetFovy();
    camera.zNear = p_camera.GetNear();
    camera.zFar = p_camera.GetFar();

    camera.viewMatrix = p_camera.GetViewMatrix();
    camera.projectionMatrixFrustum = p_camera.GetProjectionMatrix();
    // https://tomhultonharrop.com/mathematics/graphics/2023/08/06/reverse-z.html
    if (p_framedata.options.isOpengl) {
        // since we use opengl matrix for frustum culling,
        // we can use the same matrix for rendering
        camera.projectionMatrixRendering = camera.projectionMatrixFrustum;
        normalize_unit_range(camera.projectionMatrixRendering);
        reverse_z(camera.projectionMatrixRendering);
    } else {
        camera.projectionMatrixRendering = p_camera.CalcProjection();
        reverse_z(camera.projectionMatrixRendering);
    }
    camera.position = p_camera.GetPosition();

    camera.front = p_camera.GetFront();
    camera.right = p_camera.GetRight();
    camera.up = cross(camera.front, camera.right);
}

}  // namespace my
