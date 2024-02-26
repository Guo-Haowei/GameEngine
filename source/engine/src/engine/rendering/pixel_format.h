#pragma once

namespace my {

enum PixelFormat {
    FORMAT_UNKNOWN,

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

inline uint32_t channel_size(PixelFormat p_format) {
    switch (p_format) {
        case FORMAT_R8_UINT:
        case FORMAT_R8G8_UINT:
        case FORMAT_R8G8B8_UINT:
        case FORMAT_R8G8B8A8_UINT:
            return sizeof(uint8_t);
        case FORMAT_R16_FLOAT:
        case FORMAT_R16G16_FLOAT:
        case FORMAT_R16G16B16_FLOAT:
        case FORMAT_R16G16B16A16_FLOAT:
            return sizeof(uint16_t);
        case FORMAT_R32_FLOAT:
        case FORMAT_R32G32_FLOAT:
        case FORMAT_R32G32B32_FLOAT:
        case FORMAT_R32G32B32A32_FLOAT:
        case FORMAT_D32_FLOAT:
            return sizeof(float);
        default:
            CRASH_NOW();
            return 0;
    }
}

}  // namespace my
