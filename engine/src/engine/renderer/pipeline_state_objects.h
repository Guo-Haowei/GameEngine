#pragma once
#include "engine/renderer/pipeline_state.h"

namespace my {

// @TODO: make these class members
/// input layouts
static const InputLayoutDesc s_inputLayoutMesh = {
    .elements = {
        { "POSITION", 0, PixelFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, PixelFormat::R32G32B32_FLOAT, 1, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, PixelFormat::R32G32_FLOAT, 2, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, PixelFormat::R32G32B32_FLOAT, 3, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "BONEINDEX", 0, PixelFormat::R32G32B32A32_SINT, 4, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "BONEWEIGHT", 0, PixelFormat::R32G32B32A32_FLOAT, 5, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "COLOR", 0, PixelFormat::R32G32B32A32_FLOAT, 6, 0, InputClassification::PER_VERTEX_DATA, 0 },
    }
};

static const InputLayoutDesc s_inputLayoutPosition = {
    .elements = {
        { "POSITION", 0, PixelFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PER_VERTEX_DATA, 0 },
    }
};

/// rasterizer states
static const RasterizerDesc s_rasterizerFrontFace = {
    .fillMode = FillMode::SOLID,
    .cullMode = CullMode::BACK,
    .frontCounterClockwise = true,
};

static const RasterizerDesc s_rasterizerBackFace = {
    .fillMode = FillMode::SOLID,
    .cullMode = CullMode::FRONT,
    .frontCounterClockwise = true,
};

static const RasterizerDesc s_rasterizerDoubleSided = {
    .fillMode = FillMode::SOLID,
    .cullMode = CullMode::NONE,
    .frontCounterClockwise = true,
};

/// Depth stencil states
static const DepthStencilDesc s_depthStencilDefault = {
    .depthEnabled = true,
    .depthFunc = ComparisonFunc::LESS_EQUAL,
    .stencilEnabled = false,
};

static const DepthStencilDesc s_depthStencilDisabled = {
    .depthEnabled = false,
    .depthFunc = ComparisonFunc::NEVER,
    .stencilEnabled = false,
};

static const DepthStencilDesc s_depthReversedStencilDisabled = {
    .depthEnabled = true,
    .depthFunc = ComparisonFunc::GREATER_EQUAL,
    .stencilEnabled = false,
};

static const DepthStencilDesc s_depthReversedStencilEnabled = {
    .depthEnabled = true,
    .depthFunc = ComparisonFunc::GREATER_EQUAL,
    .stencilEnabled = true,
    .frontFace = {
        .stencilPassOp = StencilOp::REPLACE,
        .stencilFunc = ComparisonFunc::ALWAYS,
    },
};

static const DepthStencilDesc s_depthReversedStencilEnabledHighlight = {
    .depthEnabled = false,
    .depthFunc = ComparisonFunc::GREATER_EQUAL,
    .stencilEnabled = true,
    .frontFace = {
        .stencilFunc = ComparisonFunc::EQUAL,
    },
};

/// Blend states
static const BlendDesc s_blendStateDefault = {};

static const BlendDesc s_transparent = {
    .renderTargets = {
        {
            .blendEnabled = true,
            .blendSrc = Blend::BLEND_SRC_ALPHA,
            .blendDest = Blend::BLEND_INV_SRC_ALPHA,
            .blendOp = BlendOp::BLEND_OP_ADD,
        } }
};

static const BlendDesc s_blendStateDisable = {
    .renderTargets = {
        { .colorWriteMask = COLOR_WRITE_ENABLE_NONE },
        { .colorWriteMask = COLOR_WRITE_ENABLE_NONE },
        { .colorWriteMask = COLOR_WRITE_ENABLE_NONE },
        { .colorWriteMask = COLOR_WRITE_ENABLE_NONE },
        { .colorWriteMask = COLOR_WRITE_ENABLE_NONE },
        { .colorWriteMask = COLOR_WRITE_ENABLE_NONE },
        { .colorWriteMask = COLOR_WRITE_ENABLE_NONE },
        { .colorWriteMask = COLOR_WRITE_ENABLE_NONE },
    },
};

}  // namespace my
