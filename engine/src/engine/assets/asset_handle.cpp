#include "asset_handle.h"

#include "asset_entry.h"

namespace my {

bool AssetHandle::IsReady() const {
    return entry && entry->status == AssetStatus::Loaded;
}

[[nodiscard]] auto AssetHandle::Wait() const -> Result<AssetRef> {
    DEV_ASSERT(entry);
    return entry->Wait();
}

}  // namespace my
