#pragma once
#include "asset_meta_data.h"

namespace my {

struct IAsset;
struct BufferAsset;
struct TextAsset;
struct ImageAsset;
struct SpriteSheetAsset;

struct IAsset {
    IAsset(AssetType p_type)
        : type(p_type) {}
    virtual ~IAsset() = default;

    const AssetType type;
};

using AssetRef = std::shared_ptr<IAsset>;

using OnAssetLoadSuccessFunc = void (*)(AssetRef p_asset, void* p_userdata);

}  // namespace my
