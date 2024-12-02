#pragma once
#include "engine/assets/guid.h"
#include "engine/core/io/archive.h"
#include "engine/core/math/geomath.h"
#include "engine/renderer/gpu_resource.h"

namespace my {

enum class AssetType : uint8_t {
    IMAGE,
    BUFFER,
    SCENE,

    MESH,
    MATERIAL,
    ANIMATION,
    ARMATURE,

    COUNT,
};

// @NOTE: use file path as handle for now
// will change to guid or something
using AssetHandle = std::string;

struct AssetMetaData {
    AssetType type;
    AssetHandle handle;
    std::string path;
    // @TODO: name
};

struct IAsset {
    IAsset(AssetType p_type) : type(p_type) {}
    virtual ~IAsset() = default;

    const AssetType type;
};

// @TODO: refactor
struct File : IAsset {
    File() : IAsset(AssetType::BUFFER) {}

    std::vector<char> buffer;
};

struct Image : IAsset {
    Image() : IAsset(AssetType::IMAGE) {}

    PixelFormat format = PixelFormat::UNKNOWN;
    int width = 0;
    int height = 0;
    int num_channels = 0;
    std::vector<uint8_t> buffer;

    // @TODO: refactor
    std::shared_ptr<GpuTexture> gpu_texture;
};

#if 0
class IAsset {
public:
    virtual ~IAsset() = default;

    [[nodiscard]] virtual Result<void> Load(const std::string& p_path) = 0;
    [[nodiscard]] virtual Result<void> Save(const std::string& p_path) = 0;

    AssetType GetType() const {
        return m_type;
    }

    // protected:
    AssetType m_type;
    std::string m_name;
    Guid m_guid;
};

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
