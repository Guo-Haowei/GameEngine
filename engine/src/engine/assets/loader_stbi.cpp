#include "loader_stbi.h"

#include <stb/stb_image.h>

#include "engine/core/io/file_access.h"

namespace my {

static PixelFormat ChannelToFormat(int p_channel, bool p_is_float) {
    switch (p_channel) {
        case 1:
            return p_is_float ? PixelFormat::R32_FLOAT : PixelFormat::R8_UINT;
        case 2:
            return p_is_float ? PixelFormat::R32G32_FLOAT : PixelFormat::R8G8_UINT;
        case 3:
            return p_is_float ? PixelFormat::R32G32B32_FLOAT : PixelFormat::R8G8B8_UINT;
        case 4:
            return p_is_float ? PixelFormat::R32G32B32A32_FLOAT : PixelFormat::R8G8B8A8_UINT;
        default:
            CRASH_NOW();
            return PixelFormat::UNKNOWN;
    }
}

bool LoaderSTBIBase::LoadImpl(Image* p_image, bool p_is_float, STBILoadFunc p_func) {
    int width = 0;
    int height = 0;
    int num_channels = 0;

    auto res = FileAccess::Open(m_filePath, FileAccess::READ);
    if (!res) {
        m_error = res.error()->message;
        return false;
    }

    int req_channel = p_is_float ? 0 : 4;  // force 4 channels

    std::shared_ptr<FileAccess> file_access = *res;
    int buffer_length = (int)file_access->GetLength();
    std::vector<uint8_t> file_buffer;
    file_buffer.resize(buffer_length);
    file_access->ReadBuffer(file_buffer.data(), buffer_length);

    uint8_t* pixels = (uint8_t*)p_func(file_buffer.data(), buffer_length, &width, &height, &num_channels, req_channel);
    if (req_channel > num_channels) {
        num_channels = req_channel;
    }

    if (!pixels) {
        m_error = std::format("stbi: failed to load image '{}'", m_filePath);
        return false;
    }

    const size_t pixel_size = p_is_float ? sizeof(float) : sizeof(uint8_t);

    int num_pixels = width * height * num_channels;
    std::vector<uint8_t> buffer;
    buffer.resize(pixel_size * num_pixels);
    memcpy(buffer.data(), pixels, pixel_size * num_pixels);
    stbi_image_free(pixels);

    PixelFormat format = ChannelToFormat(num_channels, p_is_float);

    p_image->format = format;
    p_image->width = width;
    p_image->height = height;
    p_image->num_channels = num_channels;
    p_image->buffer = std::move(buffer);
    return true;
}

bool LoaderSTBI8::Load(Image* p_data) {
    return LoadImpl(p_data, false, [](uint8_t const* p_buffer, int p_len, int* p_x, int* p_y, int* p_comp, int p_req_comp) -> void* {
        return stbi_load_from_memory(p_buffer, p_len, p_x, p_y, p_comp, p_req_comp);
    });
}

bool LoaderSTBI32::Load(Image* p_data) {
    return LoadImpl(p_data, true, [](uint8_t const* p_buffer, int p_len, int* p_x, int* p_y, int* p_comp, int p_req_comp) -> void* {
        return stbi_loadf_from_memory(p_buffer, p_len, p_x, p_y, p_comp, p_req_comp);
    });
}

}  // namespace my