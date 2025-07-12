#pragma once
#include "guid.h"

namespace my {

#define ASSET_TYPE_LIST                     \
    ASSET_TYPE(Image, "image")              \
    ASSET_TYPE(Binary, "binary")            \
    ASSET_TYPE(Text, "text")                \
    ASSET_TYPE(SpriteSheet, "sprite_sheet") \
    ASSET_TYPE(Scene, "scene")

class AssetType {
public:
    enum Type : uint8_t {
#define ASSET_TYPE(ENUM, ...) ENUM,
        ASSET_TYPE_LIST
#undef ASSET_TYPE
            Count,
    };

    AssetType(Type type)
        : m_type(type) {
    }

    const char* ToString() const;

    bool operator==(const AssetType& p_rhs) const {
        return m_type == p_rhs.m_type;
    }

    uint8_t GetData() const { return m_type; }

private:
    Type m_type;
};

struct AssetMetaData {
    AssetType type{ AssetType::Count };
    Guid guid;
    std::string path;

    /// Load meta from a .meta file
    [[nodiscard]] static auto LoadMeta(std::string_view p_path) -> Result<AssetMetaData>;

    /// Create meta based on asset file
    [[nodiscard]] static auto CreateMeta(std::string_view p_path) -> std::optional<AssetMetaData>;

    /// Save .meta to disk
    [[nodiscard]] auto SaveMeta(std::string_view p_path) const -> Result<void>;
};

}  // namespace my
