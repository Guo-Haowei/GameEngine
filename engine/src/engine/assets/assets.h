#pragma once
#include "asset_interface.h"
#include "asset_handle.h"
#include "asset_meta_data.h"

#include "engine/math/box.h"
#include "engine/math/geomath.h"
#include "engine/renderer/gpu_resource.h"

namespace my {

struct BufferAsset : IAsset {
    std::vector<char> buffer;

    BufferAsset()
        : IAsset(AssetType::Binary) {}
};

struct TextAsset : IAsset {
    std::string source;

    TextAsset()
        : IAsset(AssetType::Text) {}
};

struct ImageAsset : IAsset {
    PixelFormat format = PixelFormat::UNKNOWN;
    int width = 0;
    int height = 0;
    int num_channels = 0;
    std::vector<uint8_t> buffer;

    // @TODO: write data to meta
    // @TODO: refactor
    std::shared_ptr<GpuTexture> gpu_texture;

    ImageAsset()
        : IAsset(AssetType::Image) {}
};

struct SpriteSheetAsset : IAsset {
    Guid image_id;

    int columns = 1;
    int rows = 1;
    Vector2i separation = Vector2i::Zero;
    Vector2i offset = Vector2i::Zero;

    std::vector<Rect> frames;

    /// Non serialized
    AssetHandle image_handle;

    SpriteSheetAsset()
        : IAsset(AssetType::SpriteSheet) {}

    void Serialize(std::ostream&) override;
};

}  // namespace my
