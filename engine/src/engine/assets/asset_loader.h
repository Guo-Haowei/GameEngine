#pragma once
#include "engine/assets/asset.h"

namespace my {

class IAssetLoader {
    using CreateLoaderFunc = std::unique_ptr<IAssetLoader> (*)(const AssetMetaData& p_meta);

public:
    IAssetLoader(const AssetMetaData& p_meta) : m_meta(p_meta) {}

    virtual ~IAssetLoader() = default;

    virtual IAsset* Load() = 0;

    static bool RegisterLoader(const std::string& p_extension, CreateLoaderFunc p_func);

    static std::unique_ptr<IAssetLoader> Create(const AssetMetaData& p_meta);

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

    virtual IAsset* Load() override;
};

class ImageAssetLoader : public IAssetLoader {
public:
    using IAssetLoader::IAssetLoader;

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<ImageAssetLoader>(p_meta);
    }

    virtual IAsset* Load() override;
};

}  // namespace my
