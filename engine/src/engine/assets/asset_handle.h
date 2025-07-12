#pragma once
#include "guid.h"

namespace my {

struct AssetEntry;
struct IAsset;

struct AssetHandle {
    Guid guid;
    std::shared_ptr<AssetEntry> entry;

    bool IsReady() const;
    std::shared_ptr<IAsset> Wait() const;
};

}  // namespace my

