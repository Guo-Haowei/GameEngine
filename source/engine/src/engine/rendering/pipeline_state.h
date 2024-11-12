#pragma once
#include "rendering/pixel_format.h"

namespace my {

enum class InputClassification {
    PER_VERTEX_DATA,
    PER_INSTANCE_DATA,
};

struct InputLayoutDesc {
    struct Element {
        std::string semanticName;
        uint32_t semanticIndex;
        PixelFormat format;
        uint32_t inputSlot;
        uint32_t alignedByteOffset;
        InputClassification inputSlotClass;
        uint32_t instanceDataStepRate;
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
    ComparisonFunc depthFunc;
    bool depthEnabled;
    bool stencilEnabled;

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
    // primitiveTopologyType;

    const RasterizerDesc* rasterizerDesc = nullptr;
    const DepthStencilDesc* depthStencilDesc = nullptr;
    const InputLayoutDesc* inputLayoutDesc = nullptr;
};

struct PipelineState {
    PipelineState(const InputLayoutDesc* p_input_layout_desc,
                  const RasterizerDesc* p_rasterizer_desc,
                  const DepthStencilDesc* p_depth_stencil_desc)
        : inputLayoutDesc(p_input_layout_desc),
          rasterizerDesc(p_rasterizer_desc),
          depthStencilDesc(p_depth_stencil_desc) {}

    virtual ~PipelineState() = default;

    const InputLayoutDesc* inputLayoutDesc;
    const RasterizerDesc* rasterizerDesc;
    const DepthStencilDesc* depthStencilDesc;
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

    PROGRAM_PARTICLE_INIT,
    PROGRAM_PARTICLE_KICKOFF,
    PROGRAM_PARTICLE_EMIT,
    PROGRAM_PARTICLE_SIM,
    PROGRAM_PARTICLE_RENDERING,

    PIPELINE_STATE_MAX,
};

}  // namespace my