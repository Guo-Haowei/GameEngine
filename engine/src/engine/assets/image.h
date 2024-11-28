#pragma once
#include "engine/assets/asset_handle.h"
#include "engine/rendering/gpu_resource.h"

namespace my {

class Image {
public:
    PixelFormat format = PixelFormat::UNKNOWN;
    int width = 0;
    int height = 0;
    int num_channels = 0;
    std::vector<uint8_t> buffer;

    // @TODO: refactor
    std::shared_ptr<GpuTexture> gpu_texture;
};

using ImageHandle = AssetHandle<Image>;

}  // namespace my
