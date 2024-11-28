#include "asset.h"

#include "engine/core/string/string_builder.h"

namespace my {

Result<void> MaterialAsset::Load(const std::string& p_path) {
    unused(p_path);

    return Result<void>();
}

Result<void> MaterialAsset::Save(const std::string& p_path) {
    std::string path = p_path;
    std::string guid = m_guid.ToString();
    path.append(m_name);
    path.append("_");
    path.append(guid);
    path.append(".meta");

    auto res = FileAccess::Open(path, FileAccess::WRITE);
    if (!res) {
        return HBN_ERROR(res.error());
    }
    auto file = *res;

    StringStreamBuilder builder;
    builder.Append(std::format("guid: {}\n", guid));
    builder.Append(std::format("name: {}\n", m_name));
    builder.Append(std::format("metallic: {}\n", metallic));
    builder.Append(std::format("roughness: {}\n", roughness));
    std::string buffer = builder.ToString();

    file->WriteBuffer(buffer.data(), buffer.size());

    return Result<void>();
}

}  // namespace my
