#include "renderer.h"

#include "engine/core/base/random.h"
#include "engine/core/debugger/profiler.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/math/geometry.h"
#include "engine/renderer/path_tracer/path_tracer.h"
#include "engine/renderer/render_data.h"
#include "engine/renderer/sampler.h"

// @TODO: refactor
#include "engine/math/detail/matrix.h"

#define DEFINE_DVAR
#include "graphics_dvars.h"

namespace my {
#include "shader_defines.hlsl.h"
}

namespace my::renderer {

static_assert(sizeof(KernelData) == sizeof(Vector4f) * SSAO_KERNEL_SIZE);

#define ASSERT_CAN_RECORD() DEV_ASSERT(s_glob.state == RenderState::RECORDING)

enum class RenderState {
    UNINITIALIZED = 0,
    RECORDING,
    SUBMITTING,
};

static struct {
    RenderData* renderData;
    RenderState state;
    PathTracer pt;
    // @TODO: refactor
    std::shared_ptr<GpuTexture> ssaoNoiseTexture;
    KernelData ssaoKernels;
} s_glob;

// @TODO: fix this
std::list<PointShadowHandle> s_freePointLightShadows = { 0, 1, 2, 3, 4, 5, 6, 7 };

void RegisterDvars() {
#define REGISTER_DVAR
#include "graphics_dvars.h"
}

void BeginFrame() {
    // @TODO: should be a better way
    if (s_glob.renderData) {
        delete s_glob.renderData;
        s_glob.renderData = nullptr;
    }

    RenderOptions options = {
        .isOpengl = GraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL,
        .ssaoEnabled = DVAR_GET_BOOL(gfx_ssao_enabled),
        .vxgiEnabled = false,
        .bloomEnabled = DVAR_GET_BOOL(gfx_enable_bloom),
        .debugVoxelId = DVAR_GET_INT(gfx_debug_vxgi_voxel),
        .debugBvhDepth = DVAR_GET_INT(gfx_bvh_debug),
        .voxelTextureSize = DVAR_GET_INT(gfx_voxel_size),
        .ssaoKernelRadius = DVAR_GET_FLOAT(gfx_ssao_radius),
    };

    s_glob.renderData = new RenderData(options);
    s_glob.renderData->bakeIbl = false;
    s_glob.state = RenderState::RECORDING;
}

static void PrepareDebugDraws() {
    auto& context = s_glob.renderData->drawDebugContext;
    context.drawCount = (uint32_t)context.positions.size();
}

static void PrepareImageDraws() {
    auto& buffer = s_glob.renderData->materialCache.buffer;
    auto& context = s_glob.renderData->drawImageContext;
    const uint32_t old_size = (uint32_t)buffer.size();
    const uint32_t extra_size = (uint32_t)context.size();
    s_glob.renderData->drawImageOffset = old_size;
    buffer.resize(old_size + extra_size);

    const auto resolution = DVAR_GET_IVEC2(resolution);
    for (uint32_t index = 0; index < extra_size; ++index) {
        auto& draw = context[index];
        auto& mat = buffer[old_size + index];
        auto half_ndc = draw.size / Vector2f(resolution);
        auto pos = 1.0f - half_ndc;

        mat.c_debugDrawPos.x = pos.x;
        mat.c_debugDrawPos.y = pos.y;
        mat.c_debugDrawSize.x = half_ndc.x;
        mat.c_debugDrawSize.y = half_ndc.y;
        mat.c_displayChannel = draw.mode;
        mat.c_BaseColorMapResidentHandle.Set32(static_cast<uint32_t>(draw.handle));
    }
}

void EndFrame() {
    PrepareDebugDraws();
    PrepareImageDraws();

    s_glob.state = RenderState::SUBMITTING;
}

void RequestScene(const PerspectiveCameraComponent& p_camera, Scene& p_scene) {
    HBN_PROFILE_EVENT();

    ASSERT_CAN_RECORD();

    PrepareRenderData(p_camera, p_scene, *s_glob.renderData);

    // @TODO: refactor
    s_glob.pt.Update(p_scene);
}

void RequestBakingIbl() {
    ASSERT_CAN_RECORD();

    if (DEV_VERIFY(s_glob.renderData)) {
        s_glob.renderData->bakeIbl = true;
    }
}

const RenderData* GetRenderData() {
    return s_glob.renderData;
}

void AddDebugCube(const AABB& p_aabb,
                  const Color& p_color,
                  const Matrix4x4f* p_transform) {
    ASSERT_CAN_RECORD();

    const auto& min = p_aabb.GetMin();
    const auto& max = p_aabb.GetMax();

    std::vector<Vector3f> positions;
    std::vector<uint32_t> indices;
    BoxWireFrameHelper(min, max, positions, indices);

    auto& context = s_glob.renderData->drawDebugContext;
    for (const auto& i : indices) {
        const Vector3f& pos = positions[i];
        if (p_transform) {
            const auto tmp = *p_transform * Vector4f(pos, 1.0f);
            context.positions.emplace_back(Vector3f(tmp.xyz));
        } else {
            context.positions.emplace_back(Vector3f(pos));
        }
        context.colors.emplace_back(p_color);
    }
}

void AddImage2D(GpuTexture* p_texture,
                const Vector2f& p_size,
                const Vector2f& p_position,
                int p_mode) {
    ASSERT_CAN_RECORD();

    if (DEV_VERIFY(p_texture)) {
        ImageDrawContext context = {
            .mode = p_mode,
            .handle = p_texture->GetHandle(),
            .size = p_size,
            .position = p_position,
        };
        if (GraphicsManager::GetSingleton().GetBackend() == Backend::D3D12) {
            context.handle = p_texture->GetResidentHandle();
        }
        s_glob.renderData->drawImageContext.emplace_back(context);
    }
}

PointShadowHandle AllocatePointLightShadowMap() {
    if (s_freePointLightShadows.empty()) {
        LOG_WARN("OUT OUT POINT SHADOW MAP");
        return INVALID_POINT_SHADOW_HANDLE;
    }

    int handle = s_freePointLightShadows.front();
    s_freePointLightShadows.pop_front();
    return handle;
}

void FreePointLightShadowMap(PointShadowHandle& p_handle) {
    DEV_ASSERT_INDEX(p_handle, MAX_POINT_LIGHT_SHADOW_COUNT);
    s_freePointLightShadows.push_back(p_handle);
    p_handle = INVALID_POINT_SHADOW_HANDLE;
}

// path tracer
void SetPathTracerMode(PathTracerMode p_mode) {
    s_glob.pt.SetMode(p_mode);
}

bool IsPathTracerActive() {
    return s_glob.pt.IsActive();
}

void BindPathTracerData(GraphicsManager& p_graphics_manager) {
    s_glob.pt.BindData(p_graphics_manager);
}

void UnbindPathTracerData(GraphicsManager& p_graphics_manager) {
    s_glob.pt.UnbindData(p_graphics_manager);
}

// textures generated by program
static void GenerateSsaoNoise(GraphicsManager& p_graphics_manager) {
    // generate noise texture
    std::vector<Vector2f> ssao_noise;
    for (int i = 0; i < (SSAO_NOISE_SIZE * SSAO_NOISE_SIZE); ++i) {
        Vector2f noise(Random::Float(-1.0f, 1.0f),
                       Random::Float(-1.0f, 1.0f));
        ssao_noise.emplace_back(noise);
    }

    GpuTextureDesc desc{
        .type = AttachmentType::NONE,
        .dimension = Dimension::TEXTURE_2D,
        .width = SSAO_NOISE_SIZE,
        .height = SSAO_NOISE_SIZE,
        .depth = 1,
        .mipLevels = 1,
        .arraySize = 1,
        .format = PixelFormat::R32G32_FLOAT,
        .bindFlags = BIND_SHADER_RESOURCE,
        .miscFlags = RESOURCE_MISC_NONE,
        .initialData = ssao_noise.data(),
        .name = RESOURCE_NONE,
    };

    s_glob.ssaoNoiseTexture = p_graphics_manager.CreateTexture(desc, PointWrapSampler());
}

static void GenerateSsaoKernel() {
    auto lerp = [](float a, float b, float f) {
        return a + f * (b - a);
    };

    const int kernel_size = 32;
    const float inv_kernel_size = 1.0f / kernel_size;
    for (int i = 0; i < s_glob.ssaoKernels.size(); ++i) {
        // [-1, 1], [-1, 1], [0, 1]
        Vector3f sample(Random::Float(-1.0f, 1.0f),
                        Random::Float(-1.0f, 1.0f),
                        Random::Float());

        sample = normalize(sample);
        sample *= Random::Float();
        float scale = i * inv_kernel_size;

        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        s_glob.ssaoKernels[i].xyz = sample;
    }
}

Result<void> CreateResources(GraphicsManager& p_graphics_manager) {
    GenerateSsaoKernel();
    GenerateSsaoNoise(p_graphics_manager);

    return Result<void>();
}

const KernelData& GetKernelData() {
    return s_glob.ssaoKernels;
}

const GpuTexture* GetSsaoNoiseTexture() {
    return s_glob.ssaoNoiseTexture.get();
}

}  // namespace my::renderer
