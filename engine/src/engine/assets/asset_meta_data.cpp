#include "asset_meta_data.h"

namespace my {

enum class AssetType : uint8_t;

AssetMetaData AssetMetaData::LoadMeta(const std::filesystem::path& p_path) {
    unused(p_path);

    return AssetMetaData{ .guid = Guid::Create() };
}

void AssetMetaData::SaveMeta(const std::filesystem::path& p_path) const {

    unused(p_path);
}

}  // namespace my
