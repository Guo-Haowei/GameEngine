#pragma once
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
    std::shared_ptr<IAsset> asset;
    std::atomic<AssetStatus> status;

    AssetEntry(const AssetMetaData& p_metadata)
        : metadata(p_metadata)
        , asset(nullptr)
        , status{ AssetStatus::Unloaded } {}

    AssetEntry(AssetMetaData&& p_metadata)
        : metadata(std::move(p_metadata))
        , asset(nullptr)
        , status{ AssetStatus::Unloaded } {}

    std::shared_ptr<IAsset> Wait();

    void MarkLoaded(std::shared_ptr<IAsset> p_asset);

    void MarkFailed();

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

}  // namespace my
