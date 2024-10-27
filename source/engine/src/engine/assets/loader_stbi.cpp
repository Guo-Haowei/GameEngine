#include "loader_stbi.h"

#include "assets/stb_image.h"
#include "core/io/file_access.h"

namespace my {

static PixelFormat channel_to_format(int channel, bool is_float) {
    switch (channel) {
        case 1:
            return is_float ? PixelFormat::R32_FLOAT : PixelFormat::R8_UINT;
        case 2:
            return is_float ? PixelFormat::R32G32_FLOAT : PixelFormat::R8G8_UINT;
        case 3:
            return is_float ? PixelFormat::R32G32B32_FLOAT : PixelFormat::R8G8B8_UINT;
        case 4:
            return is_float ? PixelFormat::R32G32B32A32_FLOAT : PixelFormat::R8G8B8A8_UINT;
        default:
            CRASH_NOW();
            return PixelFormat::UNKNOWN;
    }
}

bool LoaderSTBIBase::load_impl(Image* image, bool is_float, STBILoadFunc func) {
    int width = 0;
    int height = 0;
    int num_channels = 0;

    auto res = FileAccess::Open(m_file_path, FileAccess::READ);
    if (!res) {
        m_error = res.error().getMessage();
        return false;
    }

    int req_channel = is_float ? 0 : 4;  // force 4 channels

    std::shared_ptr<FileAccess> file_access = *res;
    int buffer_length = (int)file_access->GetLength();
    std::vector<uint8_t> file_buffer;
    file_buffer.resize(buffer_length);
    file_access->ReadBuffer(file_buffer.data(), buffer_length);

    uint8_t* pixels = (uint8_t*)func(file_buffer.data(), buffer_length, &width, &height, &num_channels, req_channel);
    if (req_channel > num_channels) {
        num_channels = req_channel;
    }

    if (!pixels) {
        m_error = std::format("stbi: failed to load image '{}'", m_file_path);
        return false;
    }

    const size_t pixel_size = is_float ? sizeof(float) : sizeof(uint8_t);

    int num_pixels = width * height * num_channels;
    std::vector<uint8_t> buffer;
    buffer.resize(pixel_size * num_pixels);
    memcpy(buffer.data(), pixels, pixel_size * num_pixels);
    stbi_image_free(pixels);

    PixelFormat format = channel_to_format(num_channels, is_float);

    image->format = format;
    image->width = width;
    image->height = height;
    image->num_channels = num_channels;
    image->buffer = std::move(buffer);
    return true;
}

bool LoaderSTBI8::load(Image* data) {
    return load_impl(data, false, [](uint8_t const* buffer, int len, int* x, int* y, int* comp, int req_comp) -> void* {
        return stbi_load_from_memory(buffer, len, x, y, comp, req_comp);
    });
}

bool LoaderSTBI32::load(Image* data) {
    return load_impl(data, true, [](uint8_t const* buffer, int len, int* x, int* y, int* comp, int req_comp) -> void* {
        return stbi_loadf_from_memory(buffer, len, x, y, comp, req_comp);
    });
}

}  // namespace my