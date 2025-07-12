#pragma once
#include "asset_forward.h"
#include "asset_meta_data.h"

namespace my {

struct IAsset;

enum class AssetStatus : uint8_t {
    Unloaded,
    Loading,
    Loaded,
    Failed,
};

class AssetEntry {
public:
    AssetMetaData metadata;
    AssetRef asset;
    std::atomic<AssetStatus> status;

    AssetEntry(const AssetMetaData& p_metadata)
        : metadata(p_metadata)
        , asset(nullptr)
        , status{ AssetStatus::Unloaded } {}

    AssetEntry(AssetMetaData&& p_metadata)
        : metadata(std::move(p_metadata))
        , asset(nullptr)
        , status{ AssetStatus::Unloaded } {}

    [[nodiscard]] auto Wait() -> Result<AssetRef>;

    void MarkLoaded(AssetRef p_asset);

    void MarkFailed();

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

}  // namespace my
