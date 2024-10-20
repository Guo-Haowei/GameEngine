#pragma once

// @TODO: move to shader_defines
constexpr int IMAGE_VOXEL_ALBEDO_SLOT = 0;
constexpr int IMAGE_VOXEL_NORMAL_SLOT = 1;
constexpr int IMAGE_BLOOM_DOWNSAMPLE_INPUT_SLOT = 2;
constexpr int IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT = 3;

#define RENDER_PASS_NAME_LIST          \
    RENDER_PASS_NAME(SHADOW)           \
    RENDER_PASS_NAME(GBUFFER)          \
    RENDER_PASS_NAME(LIGHTING)         \
    RENDER_PASS_NAME(BLOOM)            \
    RENDER_PASS_NAME(ENV)              \
    RENDER_PASS_NAME(HIGHLIGHT_SELECT) \
    RENDER_PASS_NAME(VOXELIZATION)     \
    RENDER_PASS_NAME(TONE)             \
    RENDER_PASS_NAME(FINAL)

namespace my {

enum class RenderPassName {
#define RENDER_PASS_NAME(name) name,
    RENDER_PASS_NAME_LIST
#undef RENDER_PASS_NAME
        COUNT,
};

inline constexpr int BLOOM_MIP_CHAIN_MAX = 7;

enum RenderTargetResourceName {
    RESOURCE_SHADOW_MAP,  // same as spot shadow, maybe make it shadow atlas?

    RESOURCE_POINT_SHADOW_MAP_0,
    RESOURCE_POINT_SHADOW_MAP_1,
    RESOURCE_POINT_SHADOW_MAP_2,
    RESOURCE_POINT_SHADOW_MAP_3,

    RESOURCE_GBUFFER_POSITION,
    RESOURCE_GBUFFER_NORMAL,
    RESOURCE_GBUFFER_BASE_COLOR,
    RESOURCE_GBUFFER_MATERIAL,
    RESOURCE_GBUFFER_DEPTH,

    RESOURCE_HIGHLIGHT_SELECT,

    RESOURCE_ENV_SKYBOX_CUBE_MAP,
    RESOURCE_ENV_SKYBOX_DEPTH,
    RESOURCE_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP,
    RESOURCE_ENV_DIFFUSE_IRRADIANCE_DEPTH,
    RESOURCE_ENV_PREFILTER_CUBE_MAP,
    RESOURCE_ENV_PREFILTER_DEPTH,

    RESOURCE_BRDF,

    RESOURCE_BLOOM_0,
    RESOURCE_BLOOM_1,
    RESOURCE_BLOOM_2,
    RESOURCE_BLOOM_3,
    RESOURCE_BLOOM_4,
    RESOURCE_BLOOM_5,
    RESOURCE_BLOOM_6,

    RESOURCE_LIGHTING,
    RESOURCE_TONE,
    RESOURCE_FINAL,

    COUNT,

    RESOURCE_BLOOM_MIN = RESOURCE_BLOOM_0,
    RESOURCE_BLOOM_MAX = RESOURCE_BLOOM_6,
};

static_assert(RESOURCE_BLOOM_MAX - RESOURCE_BLOOM_MIN + 1 == BLOOM_MIP_CHAIN_MAX);

static inline const char* renderPassNameToString(RenderPassName p_name) {
    DEV_ASSERT_INDEX(p_name, RenderPassName::COUNT);

    static const char* s_names[static_cast<int>(RenderPassName::COUNT)] = {
#define RENDER_PASS_NAME(name) #name,
        RENDER_PASS_NAME_LIST
#undef RENDER_PASS_NAME
    };
    return s_names[static_cast<int>(p_name)];
}

}  // namespace my

namespace my::rg {

class RenderGraph;

void createRenderGraphDefault(RenderGraph& p_graph);
void createRenderGraphVxgi(RenderGraph& p_graph);
// @TODO: add this
void createRenderGraphPathTracer(RenderGraph& p_graph);

}  // namespace my::rg