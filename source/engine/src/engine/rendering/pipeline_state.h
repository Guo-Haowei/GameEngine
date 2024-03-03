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
    FillMode fillMode = FillMode::SOLID;
    CullMode cullMode = CullMode::BACK;
    bool frontCounterClockwise = false;
    int depthBias = 0;
    float depthBiasClamp = 0.0f;
    float slopeScaledDepthBias = 0.0f;
    bool depthClipEnable = false;
    bool scissorEnable = false;
    bool multisampleEnable = false;
    bool antialiasedLineEnable = false;
};

// typedef struct D3D11_RASTERIZER_DESC {
//     D3D11_FILL_MODE FillMode;
//     D3D11_CULL_MODE CullMode;
//     BOOL FrontCounterClockwise;
//     INT DepthBias;
//     FLOAT DepthBiasClamp;
//     FLOAT SlopeScaledDepthBias;
//     BOOL DepthClipEnable;
//     BOOL ScissorEnable;
//     BOOL MultisampleEnable;
//     BOOL AntialiasedLineEnable;
// } D3D11_RASTERIZER_DESC;

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
};

struct PipelineState {
    PipelineState(const InputLayoutDesc* p_input_layout_desc,
                  const RasterizerDesc* p_rasterizer_desc)
        : input_layout_desc(p_input_layout_desc),
          rasterizer_desc(p_rasterizer_desc) {}

    virtual ~PipelineState() = default;

    const InputLayoutDesc* input_layout_desc;
    const RasterizerDesc* rasterizer_desc;
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
    PROGRAM_SSAO,
    PROGRAM_LIGHTING_VXGI,
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