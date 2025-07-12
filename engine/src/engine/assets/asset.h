#pragma once
#include "asset_forward.h"
#include "asset_meta_data.h"

#include "engine/math/geomath.h"
#include "engine/renderer/gpu_resource.h"

namespace my {

#define ASSET_TYPE_LIST          \
    ASSET_TYPE(Image, "image")   \
    ASSET_TYPE(Binary, "binary") \
    ASSET_TYPE(Text, "binary")   \
    ASSET_TYPE(Scene, "scene")

enum class AssetType : uint8_t {
#define ASSET_TYPE(ENUM, ...) ENUM,
    ASSET_TYPE_LIST
#undef ASSET_TYPE
        Count,
};

struct IAsset {
    IAsset(AssetType p_type)
        : type(p_type) {}
    virtual ~IAsset() = default;

    const AssetType type;
};

struct BufferAsset : IAsset {
    BufferAsset()
        : IAsset(AssetType::Binary) {}

    std::vector<char> buffer;
};

struct TextAsset : IAsset {
    TextAsset()
        : IAsset(AssetType::Text) {}

    std::string source;
};

struct ImageAsset : IAsset {
    ImageAsset()
        : IAsset(AssetType::Image) {}

    PixelFormat format = PixelFormat::UNKNOWN;
    int width = 0;
    int height = 0;
    int num_channels = 0;
    std::vector<uint8_t> buffer;

    // @TODO: refactor
    std::shared_ptr<GpuTexture> gpu_texture;
};

}  // namespace my
