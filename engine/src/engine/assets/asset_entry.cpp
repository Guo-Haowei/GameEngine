#include "asset_entry.h"

namespace my {

auto AssetEntry::Wait() -> Result<AssetRef> {
    std::unique_lock lock(m_mutex);

    m_cv.wait(lock, [this]() {
        return status == AssetStatus::Loaded || status == AssetStatus::Failed;
    });

    if (status == AssetStatus::Failed) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "failed to load {}", metadata.path);
    }

    return asset;
}

void AssetEntry::MarkLoaded(AssetRef p_asset) {
    {
        std::lock_guard lock(m_mutex);
        asset = std::move(p_asset);
        status = AssetStatus::Loaded;
    }
    m_cv.notify_all();
}

void AssetEntry::MarkFailed() {
    {
        std::lock_guard lock(m_mutex);
        status = AssetStatus::Failed;
    }
    m_cv.notify_all();
}

}  // namespace my
