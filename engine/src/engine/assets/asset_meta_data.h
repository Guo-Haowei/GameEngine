#pragma once
#include "asset_handle.h"
#include "guid.h"

namespace my {

enum class AssetType : uint8_t;

struct AssetMetaData {
    AssetType type;
    Guid guid;
    std::string path;

    // @TODO: name
    AssetHandle handle;

    static AssetMetaData LoadMeta(const std::filesystem::path& p_path);
    void SaveMeta(const std::filesystem::path& p_path) const;
};

}  // namespace my
