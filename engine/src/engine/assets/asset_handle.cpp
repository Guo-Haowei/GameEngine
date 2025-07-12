#include "asset_handle.h"

#include "asset_entry.h"

namespace my {

bool AssetHandle::IsReady() const {
    return entry && entry->status == AssetStatus::Loaded;
}

std::shared_ptr<IAsset> AssetHandle::Wait() const {
    CRASH_NOW();
    return nullptr;
}

}  // namespace my

