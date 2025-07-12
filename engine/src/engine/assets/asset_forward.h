#pragma once

namespace my {

struct IAsset;
struct BufferAsset;
struct TextAsset;
struct ImageAsset;

using AssetRef = std::shared_ptr<IAsset>;

using OnAssetLoadSuccessFunc = void (*)(AssetRef p_asset, void* p_userdata);

}  // namespace my
