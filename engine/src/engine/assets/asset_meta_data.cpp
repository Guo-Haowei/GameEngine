#include "asset_meta_data.h"

#include <yaml-cpp/yaml.h>

#include "engine/assets/asset.h"
#include "engine/core/io/file_access.h"
#include "engine/core/string/string_utils.h"

namespace my {

namespace fs = std::filesystem;

const char* AssetType::ToString() const {
    static constexpr const char* s_table[] = {
#define ASSET_TYPE(ENUM, NAME) NAME,
        ASSET_TYPE_LIST
#undef ASSET_TYPE
    };
    static_assert(array_length(s_table) == std::to_underlying(AssetType::Count));

    return s_table[std::to_underlying(m_type)];
}

static Result<AssetType> ParseAssetType(std::string_view p_string) {
#define ASSET_TYPE(ENUM, NAME) \
    if (p_string == NAME) { return AssetType::ENUM; }
    ASSET_TYPE_LIST
#undef ASSET_TYPE

    return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "invalid type {}", p_string);
}

auto AssetMetaData::LoadMeta(std::string_view p_path) -> Result<AssetMetaData> {
    std::shared_ptr<FileAccess> file;
    {
        auto res = FileAccess::Open(p_path, FileAccess::READ);
        if (!res) {
            return HBN_ERROR(res.error());
        }

        file = *res;
    }

    // @TODO: refactor this part
    const size_t size = file->GetLength();
    std::vector<char> buffer;
    buffer.resize(size);
    file->ReadBuffer(buffer.data(), size);
    buffer.push_back('\0');

    AssetMetaData meta;

    YAML::Node node = YAML::Load(buffer.data());
    {
        auto guid = node["guid"].as<std::string>();
        auto res = Guid::Parse(guid);
        if (!res) {
            return HBN_ERROR(res.error());
        }
        meta.guid = *res;
    }
    {
        auto type = node["type"].as<std::string>();
        auto res = ParseAssetType(type);
        if (!res) {
            return HBN_ERROR(res.error());
        }
        meta.type = *res;
    }
    meta.path = node["path"].as<std::string>();
    if (meta.path != p_path) {
        __debugbreak();
    }

    return meta;
}

auto AssetMetaData::CreateMeta(std::string_view p_path) -> AssetMetaData {
    auto extension = StringUtils::Extension(p_path);

    AssetType type = AssetType::Binary;
    if (extension == ".png" || extension == ".jpg") {
        type = AssetType::Image;
    } else if (extension == ".ttf") {
        type = AssetType::Binary;
    } else {
        CRASH_NOW_MSG("TODO: handle unsupported");
    }

    AssetMetaData meta;
    meta.guid = Guid::Create();
    meta.type = type;
    meta.path = p_path;

    return meta;
}

auto AssetMetaData::SaveMeta(std::string_view p_path) const -> Result<void> {
    DEV_ASSERT(!fs::exists(p_path));

    auto res = FileAccess::Open(p_path, FileAccess::WRITE);
    if (!res) {
        return HBN_ERROR(res.error());
    }

    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << "guid" << YAML::Value << guid.ToString();
    out << YAML::Key << "type" << YAML::Value << type.ToString();
    out << YAML::Key << "path" << YAML::Value << path;
    out << YAML::EndMap;

    if (!out.good()) {
        return HBN_ERROR(ErrorCode::ERR_PARSE_ERROR, "error: {}", out.GetLastError());
    }

    const char* string = out.c_str();
    size_t len = strlen(string);
    auto file = *res;
    [[maybe_unused]] size_t written = file->WriteBuffer(string, len);
    DEV_ASSERT(written == len);
    return Result<void>();
}

}  // namespace my
