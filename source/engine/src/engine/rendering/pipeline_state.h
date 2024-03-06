#pragma once
#include "rendering/pixel_format.h"

namespace my {

enum class InputClassification {
    PER_VERTEX_DATA,
    PER_INSTANCE_DATA,
};

struct InputLayoutDesc {
    struct Element {
        // @TODO: rename
        std::string semantic_name;
        uint32_t semantic_index;
        PixelFormat format;
        uint32_t input_slot;
        uint32_t aligned_byte_offset;
        InputClassification input_slot_class;
        uint32_t instance_data_step_rate;
    };

    std::vector<Element> elements;
};

enum class FillMode : uint8_t {
    WIREFRAME,
    SOLID,
};

enum class CullMode : uint8_t {
    NONE,
    FRONT,
    BACK,
    FRONT_AND_BACK,
};

struct RasterizerDesc {
    FillMode fill_mode = FillMode::SOLID;
    CullMode cull_mode = CullMode::BACK;
    bool front_counter_clockwise = false;
    int depth_bias = 0;
    float depth_bias_clamp = 0.0f;
    float slope_scaled_depth_bias = 0.0f;
    bool depth_clip_enable = false;
    bool scissor_enable = false;
    bool multisample_enable = false;
    bool antialiased_line_enable = false;
};

enum class ComparisonFunc : uint8_t {
    NEVER,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL,
    ALWAYS,
};

enum class DepthStencilOpDesc : uint8_t {
    ALWAYS,
    Z_PASS,
    EQUAL,
};

struct DepthStencilDesc {
    ComparisonFunc depth_func;
    bool depth_enabled;
    bool stencil_enabled;

    DepthStencilOpDesc op = DepthStencilOpDesc::ALWAYS;
};

struct ShaderMacro {
    const char* name;
    const char* value;
};

struct PipelineCreateInfo {
    std::string_view vs;
    std::string_view ps;
    std::string_view gs;
    std::string_view cs;
    std::vector<ShaderMacro> defines;

    const InputLayoutDesc* input_layout_desc = nullptr;
    const RasterizerDesc* rasterizer_desc = nullptr;
    const DepthStencilDesc* depth_stencil_desc = nullptr;
};

struct PipelineState {
    PipelineState(const InputLayoutDesc* p_input_layout_desc,
                  const RasterizerDesc* p_rasterizer_desc,
                  const DepthStencilDesc* p_depth_stencil_desc)
        : input_layout_desc(p_input_layout_desc),
          rasterizer_desc(p_rasterizer_desc),
          depth_stencil_desc(p_depth_stencil_desc) {}

    virtual ~PipelineState() = default;

    const InputLayoutDesc* input_layout_desc;
    const RasterizerDesc* rasterizer_desc;
    const DepthStencilDesc* depth_stencil_desc;
};

enum PipelineStateName {
    // @TODO: split render passes to static and dynamic
    PROGRAM_DPETH_STATIC,
    PROGRAM_DPETH_ANIMATED,
    PROGRAM_POINT_SHADOW_STATIC,
    PROGRAM_POINT_SHADOW_ANIMATED,
    PROGRAM_GBUFFER_STATIC,
    PROGRAM_GBUFFER_ANIMATED,
    PROGRAM_VOXELIZATION_STATIC,
    PROGRAM_VOXELIZATION_ANIMATED,
    PROGRAM_VOXELIZATION_POST,
    PROGRAM_HIGHLIGHT,
    PROGRAM_SSAO,
    PROGRAM_LIGHTING,
    PROGRAM_BLOOM_SETUP,
    PROGRAM_BLOOM_DOWNSAMPLE,
    PROGRAM_BLOOM_UPSAMPLE,
    PROGRAM_TONE,
    PROGRAM_DEBUG_VOXEL,
    PROGRAM_ENV_SKYBOX_TO_CUBE_MAP,
    PROGRAM_DIFFUSE_IRRADIANCE,
    PROGRAM_PREFILTER,
    PROGRAM_ENV_SKYBOX,
    PROGRAM_BRDF,
    PROGRAM_BILLBOARD,
    PROGRAM_IMAGE_2D,
    PIPELINE_STATE_MAX,
};

}  // namespace my