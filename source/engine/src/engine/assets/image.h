#pragma once
#include "assets/asset_handle.h"
#include "rendering/pixel_format.h"
#include "rendering/texture.h"

namespace my {

class Image {
public:
    Image(const std::string& p_debug_name) : debug_name(p_debug_name) {}
    std::string debug_name;

    PixelFormat format = FORMAT_UNKNOWN;
    int width = 0;
    int height = 0;
    int num_channels = 0;
    std::vector<uint8_t> buffer;

    std::shared_ptr<Texture> gpu_texture;
};

using ImageHandle = AssetHandle<Image>;

}  // namespace my
