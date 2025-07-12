#pragma once
#include "asset_forward.h"
#include "asset_meta_data.h"

#include "engine/math/geomath.h"
#include "engine/renderer/gpu_resource.h"

namespace my {

enum class AssetType : uint8_t {
    NONE,
    IMAGE,
    BUFFER,
    TEXT,
    SCENE,

    COUNT,
};

struct IAsset {
    IAsset(AssetType p_type)
        : type(p_type) {}
    virtual ~IAsset() = default;

    const AssetType type;
};

struct BufferAsset : IAsset {
    BufferAsset()
        : IAsset(AssetType::BUFFER) {}

    std::vector<char> buffer;
};

struct TextAsset : IAsset {
    TextAsset()
        : IAsset(AssetType::TEXT) {}

    std::string source;
};

struct ImageAsset : IAsset {
    ImageAsset()
        : IAsset(AssetType::IMAGE) {}

    PixelFormat format = PixelFormat::UNKNOWN;
    int width = 0;
    int height = 0;
    int num_channels = 0;
    std::vector<uint8_t> buffer;

    // @TODO: refactor
    std::shared_ptr<GpuTexture> gpu_texture;
};

}  // namespace my
