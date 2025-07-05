#pragma once
#include "engine/renderer/pixel_format.h"
#include "engine/renderer/render_graph/render_graph_resources.h"

// @TODO: move to shader_defines
constexpr int IMAGE_VOXEL_ALBEDO_SLOT = 0;
constexpr int IMAGE_VOXEL_NORMAL_SLOT = 1;
constexpr int IMAGE_BLOOM_DOWNSAMPLE_INPUT_SLOT = 2;

#define RENDER_PASS_NAME_LIST      \
    RENDER_PASS_NAME(FORWARD)      \
    RENDER_PASS_NAME(PREPASS)      \
    RENDER_PASS_NAME(SHADOW)       \
    RENDER_PASS_NAME(GBUFFER)      \
    RENDER_PASS_NAME(SSAO)         \
    RENDER_PASS_NAME(LIGHTING)     \
    RENDER_PASS_NAME(BLOOM)        \
    RENDER_PASS_NAME(ENV)          \
    RENDER_PASS_NAME(OUTLINE)      \
    RENDER_PASS_NAME(VOXELIZATION) \
    RENDER_PASS_NAME(TONE)         \
    RENDER_PASS_NAME(PATH_TRACER)  \
    RENDER_PASS_NAME(FINAL)

// @TODO: RESOURCE_SHADOW_MAP same as spot shadow, maybe make it shadow atlas?
#define RENDER_TARGET_RESOURCE_NAME_LIST                                  \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_NONE)                            \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_SHADOW_MAP)                      \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_POINT_SHADOW_CUBE_ARRAY)         \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_GBUFFER_NORMAL)                  \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_GBUFFER_BASE_COLOR)              \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_GBUFFER_MATERIAL)                \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_GBUFFER_DEPTH)                   \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_SSAO)                            \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_OUTLINE_SELECT)                  \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_ENV_SKYBOX_CUBE_MAP)             \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP) \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_ENV_PREFILTER_CUBE_MAP)          \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_BLOOM_0)                         \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_BLOOM_1)                         \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_BLOOM_2)                         \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_BLOOM_3)                         \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_BLOOM_4)                         \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_BLOOM_5)                         \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_BLOOM_6)                         \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_LIGHTING)                        \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_TONE)                            \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_VOXEL_LIGHTING)                  \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_VOXEL_NORMAL)                    \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_PATH_TRACER)                     \
    RENDER_TARGET_RESOURCE_NAME(RESOURCE_FINAL)

namespace my {

enum class RenderPassName {
#define RENDER_PASS_NAME(name) name,
    RENDER_PASS_NAME_LIST
#undef RENDER_PASS_NAME
        COUNT,
};

enum RenderTargetResourceName : uint8_t {
#define RENDER_TARGET_RESOURCE_NAME(Name) Name,
    RENDER_TARGET_RESOURCE_NAME_LIST
#undef RENDER_TARGET_RESOURCE_NAME

        COUNT,

    RESOURCE_BLOOM_MIN = RESOURCE_BLOOM_0,
    RESOURCE_BLOOM_MAX = RESOURCE_BLOOM_6,
};

static_assert(RESOURCE_BLOOM_MAX - RESOURCE_BLOOM_MIN + 1 == BLOOM_MIP_CHAIN_MAX);

static inline const char* RenderPassNameToString(RenderPassName p_name) {
    DEV_ASSERT_INDEX(p_name, RenderPassName::COUNT);

    static const char* s_names[static_cast<int>(RenderPassName::COUNT)] = {
#define RENDER_PASS_NAME(name) #name,
        RENDER_PASS_NAME_LIST
#undef RENDER_PASS_NAME
    };
    return s_names[static_cast<int>(p_name)];
}

static inline const char* RenderTargetResourceNameToString(RenderTargetResourceName p_name) {
    DEV_ASSERT_INDEX(p_name, RenderTargetResourceName::COUNT);

    static const char* s_names[static_cast<int>(RenderTargetResourceName::COUNT)] = {
#define RENDER_TARGET_RESOURCE_NAME(name) #name,
        RENDER_TARGET_RESOURCE_NAME_LIST
#undef RENDER_TARGET_RESOURCE_NAME
    };
    return s_names[static_cast<int>(p_name)];
}

}  // namespace my
