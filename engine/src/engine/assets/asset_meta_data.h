#pragma once
#include "guid.h"

namespace my {

enum class AssetType : uint8_t;

struct AssetMetaData {
    AssetType type;
    Guid guid;
    std::string path;

    /// Load meta from a .meta file
    [[nodiscard]] static auto LoadMeta(std::string_view p_path) -> Result<AssetMetaData>;

    /// Create meta based on asset file
    [[nodiscard]] static auto CreateMeta(std::string_view p_path) -> AssetMetaData;

    /// Save .meta to disk
    [[nodiscard]] auto SaveMeta(std::string_view p_path) const -> Result<void>;
};

}  // namespace my
