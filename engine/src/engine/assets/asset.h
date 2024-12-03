#pragma once
#include "engine/assets/asset_handle.h"
#include "engine/core/io/archive.h"
#include "engine/core/math/geomath.h"
#include "engine/renderer/gpu_resource.h"

namespace my {

enum class AssetType : uint8_t {
    IMAGE,
    BUFFER,
    SCENE,
    MATERIAL,

    // MESH,
    // ANIMATION,
    // ARMATURE,
    COUNT,
};

struct IAsset {
    struct Meta {
        AssetType type;
        AssetHandle handle;
        std::string path;
        // @TODO: name
    };

    IAsset(AssetType p_type) : type(p_type) {}
    virtual ~IAsset() = default;

    const AssetType type;

    // @TODO: find a more efficient way
    Meta meta;
};

using OnAssetLoadSuccessFunc = void (*)(IAsset* p_asset, void* p_userdata);

struct BufferAsset : IAsset {
    BufferAsset() : IAsset(AssetType::BUFFER) {}

    std::vector<char> buffer;
};

struct ImageAsset : IAsset {
    ImageAsset() : IAsset(AssetType::IMAGE) {}

    PixelFormat format = PixelFormat::UNKNOWN;
    int width = 0;
    int height = 0;
    int num_channels = 0;
    std::vector<uint8_t> buffer;

    // @TODO: refactor
    std::shared_ptr<GpuTexture> gpu_texture;
};

#if 0

class MaterialAsset : public IAsset {
public:
    enum {
        TEXTURE_BASE = 0,
        TEXTURE_NORMAL,
        TEXTURE_METALLIC_ROUGHNESS,
    };

    // @TODO: reflection to register properties?
    Vector4f baseColor;
    Vector3f emissive;
    float metallic;
    float roughness;
    //////////////////////////////////////

    Result<void> Load(const std::string& p_path) override;
    Result<void> Save(const std::string& p_path) override;
};
#endif

}  // namespace my
