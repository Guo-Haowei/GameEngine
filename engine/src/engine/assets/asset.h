#pragma once
#include "asset_interface.h"
#include "asset_meta_data.h"

#include "engine/math/geomath.h"
#include "engine/renderer/gpu_resource.h"

namespace my {

struct BufferAsset : IAsset {
    BufferAsset()
        : IAsset(AssetType::Binary) {}

    std::vector<char> buffer;
};

struct TextAsset : IAsset {
    TextAsset()
        : IAsset(AssetType::Text) {}

    std::string source;
};

struct ImageAsset : IAsset {
    ImageAsset()
        : IAsset(AssetType::Image) {}

    PixelFormat format = PixelFormat::UNKNOWN;
    int width = 0;
    int height = 0;
    int num_channels = 0;
    std::vector<uint8_t> buffer;

    // @TODO: refactor
    std::shared_ptr<GpuTexture> gpu_texture;
};

}  // namespace my
