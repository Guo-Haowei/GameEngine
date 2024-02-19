#pragma once
#include "assets/asset_handle.h"
#include "rendering/pixel_format.h"

namespace my {

// @TODO: refactor
struct Texture {
    uint32_t handle;
    uint64_t resident_handle;
};

class Image {
public:
    PixelFormat format = FORMAT_UNKNOWN;
    int width = 0;
    int height = 0;
    int num_channels = 0;
    std::vector<uint8_t> buffer;

    // @TODO: refactor
    Texture texture;
};

}  // namespace my
