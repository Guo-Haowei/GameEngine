#pragma once
#include "asset_interface.h"
#include "guid.h"

namespace my {

class AssetEntry;
struct IAsset;

struct AssetHandle {
    Guid guid;
    std::shared_ptr<AssetEntry> entry;

    bool IsValid() const { return !!entry; }

    bool IsReady() const;
    [[nodiscard]] auto Wait() const -> Result<AssetRef>;

    template<typename T>
    [[nodiscard]] auto Wait() -> Result<std::shared_ptr<T>> {
        auto res = Wait();
        if (!res) {
            return HBN_ERROR(res.error());
        }

        AssetRef ptr = *res;
        return std::dynamic_pointer_cast<T>(ptr);
    }
};

}  // namespace my
