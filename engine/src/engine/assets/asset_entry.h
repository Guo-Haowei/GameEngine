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

struct AssetEntry {
    AssetMetaData metadata;
    std::shared_ptr<IAsset> asset;
    std::atomic<AssetStatus> status{ AssetStatus::Unloaded };

    std::mutex mutex;

    AssetEntry(const AssetMetaData& p_metadata)
        : metadata(p_metadata)
        , asset(nullptr)
        , status{ AssetStatus::Unloaded } {}

    AssetEntry(AssetMetaData&& p_metadata)
        : metadata(std::move(p_metadata))
        , asset(nullptr)
        , status{ AssetStatus::Unloaded } {}
};

}  // namespace my
