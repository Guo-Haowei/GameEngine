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

constexpr uint8_t DEFAULT_STENCIL_READ_MASK = 0xff;
constexpr uint8_t DEFAULT_STENCIL_WRITE_MASK = 0xff;

struct StencilOpDesc {
    StencilOp stencilFailOp{ StencilOp::KEEP };
    StencilOp stencilDepthFailOp{ StencilOp::KEEP };
    StencilOp stencilPassOp{ StencilOp::KEEP };
    ComparisonFunc stencilFunc{ ComparisonFunc::ALWAYS };
};

struct DepthStencilDesc {
    bool depthEnabled;
    ComparisonFunc depthFunc;

    bool stencilEnabled;
    uint8_t stencilReadMask{ DEFAULT_STENCIL_READ_MASK };
    uint8_t stencilWriteMask{ DEFAULT_STENCIL_WRITE_MASK };
    StencilOpDesc frontFace{};
    StencilOpDesc backFace{};
};

struct ShaderMacro {
    const char* name;
    const char* value;
};

enum class PipelineStateType {
    GRAPHICS,
    COMPUTE,
};

struct PipelineStateDesc {
    PipelineStateType type{ PipelineStateType::GRAPHICS };

    std::string_view vs;
    std::string_view ps;
    std::string_view gs;
    std::string_view cs;

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
    PSO_DPETH,
    PSO_POINT_SHADOW,
    PSO_GBUFFER,
    PSO_VOXELIZATION,
    PSO_VOXELIZATION_PRE,
    PSO_VOXELIZATION_POST,
    PSO_HIGHLIGHT,
    PSO_LIGHTING,
    PSO_BLOOM_SETUP,
    PSO_BLOOM_DOWNSAMPLE,
    PSO_BLOOM_UPSAMPLE,
    PSO_TONE,
    PSO_DEBUG_VOXEL,
    PSO_ENV_SKYBOX_TO_CUBE_MAP,
    PSO_DIFFUSE_IRRADIANCE,
    PSO_PREFILTER,
    PSO_ENV_SKYBOX,
    PSO_BRDF,
    PSO_BILLBOARD,
    PSO_RW_TEXTURE_2D,

    PSO_PARTICLE_INIT,
    PSO_PARTICLE_KICKOFF,
    PSO_PARTICLE_EMIT,
    PSO_PARTICLE_SIM,
    PSO_PARTICLE_RENDERING,

    PSO_MAX,
};

}  // namespace my