#pragma once
#include "engine/assets/asset.h"
#include "engine/renderer/graphics_dvars.h"

namespace my {

struct IAsset;

class IAssetLoader {
    using CreateLoaderFunc = std::unique_ptr<IAssetLoader> (*)(const AssetMetaData& p_meta);

public:
    IAssetLoader(const AssetMetaData& p_meta);

    virtual ~IAssetLoader() = default;

    [[nodiscard]] virtual auto Load() -> Result<IAsset*> = 0;

    static bool RegisterLoader(const std::string& p_extension, CreateLoaderFunc p_func);

    static std::unique_ptr<IAssetLoader> Create(const AssetMetaData& p_meta);

    inline static std::map<std::string, CreateLoaderFunc> s_loaderCreator;

protected:
    const AssetMetaData& m_meta;

    std::string m_fileName;
    std::string m_filePath;
    std::string m_basePath;
    std::string m_extension;
};

class BufferAssetLoader : public IAssetLoader {
public:
    using IAssetLoader::IAssetLoader;

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<BufferAssetLoader>(p_meta);
    }

    auto Load() -> Result<IAsset*> override;
};

class TextAssetLoader : public IAssetLoader {
public:
    using IAssetLoader::IAssetLoader;

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<TextAssetLoader>(p_meta);
    }

    auto Load() -> Result<IAsset*> override;
};

class ImageAssetLoader : public IAssetLoader {
public:
    ImageAssetLoader(const AssetMetaData& p_meta, uint32_t p_size)
        : IAssetLoader(p_meta), m_size(p_size) {}

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<ImageAssetLoader>(p_meta, 1);
    }

    static std::unique_ptr<IAssetLoader> CreateLoaderF(const AssetMetaData& p_meta) {
        return std::make_unique<ImageAssetLoader>(p_meta, 4);
    }

    auto Load() -> Result<IAsset*> override;

protected:
    const uint32_t m_size;
};

class SceneLoader : public IAssetLoader {
public:
    using IAssetLoader::IAssetLoader;

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<SceneLoader>(p_meta);
    }

    auto Load() -> Result<IAsset*> override;
};

class TextSceneLoader : public IAssetLoader {
public:
    using IAssetLoader::IAssetLoader;

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<TextSceneLoader>(p_meta);
    }

    auto Load() -> Result<IAsset*> override;
};

}  // namespace my
