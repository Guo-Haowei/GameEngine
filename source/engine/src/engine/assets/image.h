#pragma once
#include "core/base/rid.h"

namespace my {

enum PixelFormat {
    FORMAT_R8_UINT,
    FORMAT_R8G8_UINT,
    FORMAT_R8G8B8_UINT,
    FORMAT_R8G8B8A8_UINT,

    FORMAT_R16_FLOAT,
    FORMAT_R16G16_FLOAT,
    FORMAT_R16G16B16_FLOAT,
    FORMAT_R16G16B16A16_FLOAT,

    FORMAT_R32_FLOAT,
    FORMAT_R32G32_FLOAT,
    FORMAT_R32G32B32_FLOAT,
    FORMAT_R32G32B32A32_FLOAT,

    FORMAT_D32_FLOAT,
};

class Image {
public:
    Image(PixelFormat p_format, int p_width, int p_height, int p_num_channels, std::vector<uint8_t>& p_buffer)
        : format(p_format),
          width(p_width),
          height(p_height),
          num_channels(p_num_channels),
          buffer(std::move(p_buffer)) {
    }

    PixelFormat format;
    const int width;
    const int height;
    const int num_channels;
    std::vector<uint8_t> buffer;

    RID gpu_resource;
};

}  // namespace my
