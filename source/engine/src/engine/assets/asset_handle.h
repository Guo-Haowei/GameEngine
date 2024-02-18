#pragma once

namespace my {

class Image;

enum AssetLoadingState : uint32_t {
    ASSET_STATE_LOADING,
    ASSET_STATE_READY,
};

template<typename T>
struct AssetHandle {
    std::atomic<T*> data = nullptr;
    std::atomic<AssetLoadingState> state = ASSET_STATE_LOADING;
};

using ImageHandle = AssetHandle<Image>;

}  // namespace my