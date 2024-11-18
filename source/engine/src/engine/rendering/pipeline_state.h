#pragma once
#include "rendering/graphics_defines.h"
#include "rendering/pixel_format.h"

namespace my {

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

struct PipelineStateDesc {
    std::string_view vs;
    std::string_view ps;
    std::string_view gs;
    std::string_view cs;
    std::vector<ShaderMacro> defines;

    PrimitiveTopology primitiveTopology{ PrimitiveTopology::TRIANGLE };
    const RasterizerDesc* rasterizerDesc{ nullptr };
    const DepthStencilDesc* depthStencilDesc{ nullptr };
    const InputLayoutDesc* inputLayoutDesc{ nullptr };

    uint32_t numRenderTargets{ 0 };
    PixelFormat rtvFormats[8];
    PixelFormat dsvFormat;
};

struct PipelineState {
    PipelineState(const PipelineStateDesc& p_desc) : desc(p_desc) {}

    virtual ~PipelineState() = default;

    const PipelineStateDesc desc;
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