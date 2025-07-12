#pragma once
#include "guid.h"

namespace my {

class AssetEntry;
struct IAsset;

struct AssetHandle {
    Guid guid;
    std::shared_ptr<AssetEntry> entry;

    bool IsReady() const;
    std::shared_ptr<IAsset> Wait() const;

    template<typename T>
    std::shared_ptr<T> Wait() {
        std::shared_ptr<IAsset> ptr = Wait();
        return std::dynamic_pointer_cast<T>(ptr);
    }
};

}  // namespace my
