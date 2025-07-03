#include "asset_loader.h"

#include "tinygltf/stb_image.h"

#include "engine/assets/asset.h"
#include "engine/core/io/file_access.h"
#include "engine/core/string/string_utils.h"
#include "engine/renderer/pixel_format.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_serialization.h"

namespace my {

IAssetLoader::IAssetLoader(const IAsset::Meta& p_meta) : m_meta(p_meta) {
    m_filePath = m_meta.path;
    std::filesystem::path system_path{ m_filePath };
    m_fileName = system_path.filename().string();
    m_basePath = system_path.remove_filename().string();
}

bool IAssetLoader::RegisterLoader(const std::string& p_extension, CreateLoaderFunc p_func) {
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

std::unique_ptr<IAssetLoader> IAssetLoader::Create(const IAsset::Meta& p_meta) {
    std::string_view extension = StringUtils::Extension(p_meta.path);
    auto it = s_loaderCreator.find(std::string(extension));
    if (it == s_loaderCreator.end()) {
        return nullptr;
    }
    return it->second(p_meta);
}

auto BufferAssetLoader::Load() -> Result<IAsset*> {
    auto res = FileAccess::Open(m_meta.path, FileAccess::READ);
    if (!res) {
        return HBN_ERROR(res.error());
    }

    std::shared_ptr<FileAccess> file_access = *res;

    const size_t size = file_access->GetLength();

    std::vector<char> buffer;
    buffer.resize(size);
    file_access->ReadBuffer(buffer.data(), size);
    auto file = new BufferAsset;
    file->buffer = std::move(buffer);
    return file;
}

auto TextAssetLoader::Load() -> Result<IAsset*> {
    auto res = FileAccess::Open(m_meta.path, FileAccess::READ);
    if (!res) {
        return HBN_ERROR(res.error());
    }

    std::shared_ptr<FileAccess> file_access = *res;

    const size_t size = file_access->GetLength();
    std::vector<char> buffer;
    buffer.resize(size);
    file_access->ReadBuffer(buffer.data(), size);

    auto file = new TextAsset;
    file->source = std::string(buffer.begin(), buffer.end());
    return file;
}

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

auto ImageAssetLoader::Load() -> Result<IAsset*> {
    auto res = FileAccess::Open(m_meta.path, FileAccess::READ);
    if (!res) {
        return HBN_ERROR(res.error());
    }

    const bool is_float = m_size == 4;

    std::shared_ptr<FileAccess> file = *res;
    const size_t size = file->GetLength();
    std::vector<uint8_t> file_buffer;
    file_buffer.resize(size);
    file->ReadBuffer(file_buffer.data(), size);

    int width = 0;
    int height = 0;
    int num_channels = 0;
    // const int req_channel = is_float ? 0 : 4;
    // @TODO: fix this
    const int req_channel = 4;

    uint8_t* pixels = nullptr;
    if (is_float) {
        pixels = (uint8_t*)stbi_loadf_from_memory(file_buffer.data(),
                                                  (uint32_t)size,
                                                  &width,
                                                  &height,
                                                  &num_channels,
                                                  req_channel);
    } else {
        pixels = (uint8_t*)stbi_load_from_memory(file_buffer.data(),
                                                 (uint32_t)size,
                                                 &width,
                                                 &height,
                                                 &num_channels,
                                                 req_channel);
    }

    if (!pixels) {
        return HBN_ERROR(ErrorCode::ERR_PARSE_ERROR, "failed to parse file '{}'", m_meta.path);
    }

    if (req_channel > num_channels) {
        num_channels = req_channel;
    }

    const uint32_t pixel_size = is_float ? sizeof(float) : sizeof(uint8_t);

    int num_pixels = width * height * num_channels;
    std::vector<uint8_t> buffer;
    buffer.resize(pixel_size * num_pixels);
    memcpy(buffer.data(), pixels, pixel_size * num_pixels);
    stbi_image_free(pixels);

    PixelFormat format = ChannelToFormat(num_channels, is_float);

    auto p_image = new ImageAsset;
    p_image->format = format;
    p_image->width = width;
    p_image->height = height;
    p_image->num_channels = num_channels;
    p_image->buffer = std::move(buffer);
    return p_image;
}

// @TODO: use same loader for both
auto SceneLoader::Load() -> Result<IAsset*> {
    Scene* scene = new Scene;
    auto res = LoadSceneBinary(m_filePath, *scene);
    if (!res) {
        delete scene;
        return HBN_ERROR(res.error());
    }

    scene->m_replace = true;
    return scene;
}

// @TODO: use same loader for both
auto TextSceneLoader::Load() -> Result<IAsset*> {
    Scene* scene = new Scene;
    auto res = LoadSceneText(m_filePath, *scene);
    if (!res) {
        delete scene;
        return HBN_ERROR(res.error());
    }

    scene->m_replace = true;
    return scene;
}

}  // namespace my
