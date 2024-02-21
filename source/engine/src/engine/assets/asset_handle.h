#pragma once

namespace my {

enum AssetLoadingState : uint32_t {
    ASSET_STATE_LOADING,
    ASSET_STATE_READY,
};

template<typename T>
class AssetHandle {
public:
    std::atomic<T*> data = nullptr;
    std::atomic<AssetLoadingState> state = ASSET_STATE_LOADING;

    T* get() {
        return state == ASSET_STATE_READY ? data.load() : nullptr;
    }

    bool set(T* p_data) {
        DEV_ASSERT(p_data);
        DEV_ASSERT(state != ASSET_STATE_READY);
        data.store(p_data);
        state = ASSET_STATE_READY;
        return true;
    }
};

}  // namespace my