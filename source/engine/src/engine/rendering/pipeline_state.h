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
};

struct PipelineState {
    virtual ~PipelineState() = default;
};

enum PipelineStateName {
    // @TODO: split render passes to static and dynamic
    PROGRAM_BASE_COLOR_STATIC,
    PROGRAM_BASE_COLOR_ANIMATED,
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