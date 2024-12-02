#pragma once
#include "engine/assets/asset.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/scene/scene.h"

namespace my {

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

class ImageAssetLoader : public IAssetLoader {
public:
    using IAssetLoader::IAssetLoader;

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<ImageAssetLoader>(p_meta);
    }

    auto Load() -> Result<IAsset*> override;
};

class SceneLoader : public IAssetLoader {
public:
    using IAssetLoader::IAssetLoader;

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<SceneLoader>(p_meta);
    }

    auto Load() -> Result<IAsset*> override;
};

class LuaSceneLoader : public IAssetLoader {
public:
    using IAssetLoader::IAssetLoader;

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<LuaSceneLoader>(p_meta);
    }

    auto Load() -> Result<IAsset*> override;
};

}  // namespace my
