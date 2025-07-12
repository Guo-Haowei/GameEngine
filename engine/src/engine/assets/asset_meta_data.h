#pragma once
#include "guid.h"

namespace my {

enum class AssetType : uint8_t;

struct AssetMetaData {
    AssetType type;
    Guid guid;
    std::string path;

    static AssetMetaData LoadMeta(const std::filesystem::path& p_path);
    void SaveMeta(const std::filesystem::path& p_path) const;
};

}  // namespace my
