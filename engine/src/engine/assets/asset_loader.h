#pragma once
#include "engine/assets/asset.h"
#include "engine/core/string/string_utils.h"
#include "engine/renderer/pixel_format.h"

// @TODO: remove
#include <stb/stb_image.h>

namespace my {

class IAssetLoader {
    using CreateLoaderFunc = std::unique_ptr<IAssetLoader> (*)(const AssetMetaData& p_meta);

public:
    IAssetLoader(const AssetMetaData& p_meta) : m_meta(p_meta) {}

    virtual ~IAssetLoader() = default;

    virtual IAsset* Load() = 0;

    static bool RegisterLoader(const std::string& p_extension, CreateLoaderFunc p_func) {
        DEV_ASSERT(!p_extension.empty());
        DEV_ASSERT(p_func);
        auto it = s_loaderCreator.find(p_extension);
        if (it != s_loaderCreator.end()) {
            LOG_WARN("Already exists a loader for p_extension '{}'", p_extension);
            it->second = p_func;
        } else {
            s_loaderCreator[p_extension] = p_func;
        }
        return true;
    }

    static std::unique_ptr<IAssetLoader> Create(const AssetMetaData& p_meta) {
        std::string_view extension = StringUtils::Extension(p_meta.path);
        auto it = s_loaderCreator.find(std::string(extension));
        if (it == s_loaderCreator.end()) {
            return nullptr;
        }
        return it->second(p_meta);
    }

    inline static std::map<std::string, CreateLoaderFunc> s_loaderCreator;

protected:
    const AssetMetaData& m_meta;
};

class BufferAssetLoader : public IAssetLoader {
public:
    using IAssetLoader::IAssetLoader;

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<BufferAssetLoader>(p_meta);
    }

    virtual IAsset* Load() override {
        auto res = FileAccess::Open(m_meta.path, FileAccess::READ);
        if (!res) {
            LOG_FATAL("TODO: handle error");
            return nullptr;
        }

        std::shared_ptr<FileAccess> file_access = *res;

        const size_t size = file_access->GetLength();

        std::vector<char> buffer;
        buffer.resize(size);
        file_access->ReadBuffer(buffer.data(), size);
        auto file = new File;
        file->buffer = std::move(buffer);
        return file;
    }
};

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

class ImageAssetLoader : public IAssetLoader {
public:
    using IAssetLoader::IAssetLoader;

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<ImageAssetLoader>(p_meta);
    }

    virtual IAsset* Load() override {
        auto res = FileAccess::Open(m_meta.path, FileAccess::READ);
        if (!res) {
            LOG_FATAL("TODO: handle error");
            return nullptr;
        }

        const bool p_is_float = false;

        std::shared_ptr<FileAccess> file = *res;
        const size_t size = file->GetLength();
        std::vector<uint8_t> file_buffer;
        file_buffer.resize(size);
        file->ReadBuffer(file_buffer.data(), size);

        int width = 0;
        int height = 0;
        int num_channels = 0;
        int req_channel = 4;

        uint8_t* pixels = (uint8_t*)stbi_load_from_memory(file_buffer.data(),
                                                          (uint32_t)size,
                                                          &width,
                                                          &height,
                                                          &num_channels,
                                                          req_channel);

        if (!pixels) {
            LOG_FATAL("TODO: handle error");
            return nullptr;
        }

        if (req_channel > num_channels) {
            num_channels = req_channel;
        }

        const uint32_t pixel_size = p_is_float ? sizeof(float) : sizeof(uint8_t);

        int num_pixels = width * height * num_channels;
        std::vector<uint8_t> buffer;
        buffer.resize(pixel_size * num_pixels);
        memcpy(buffer.data(), pixels, pixel_size * num_pixels);
        stbi_image_free(pixels);

        PixelFormat format = ChannelToFormat(num_channels, p_is_float);

        auto p_image = new Image;
        p_image->format = format;
        p_image->width = width;
        p_image->height = height;
        p_image->num_channels = num_channels;
        p_image->buffer = std::move(buffer);
        return p_image;
    }
};

}  // namespace my
