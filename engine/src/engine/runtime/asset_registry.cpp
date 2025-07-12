#include "asset_registry.h"

#include <fstream>
#include <latch>
#include <yaml-cpp/yaml.h>

#include "engine/core/string/string_utils.h"
#include "engine/runtime/application.h"
#include "engine/runtime/asset_manager.h"

namespace my {

namespace fs = std::filesystem;

auto AssetRegistry::InitializeImpl() -> Result<void> {
    fs::path assets_root = fs::path{ m_app->GetResourceFolder() };

    struct Pair {
        bool has_meta;
        bool has_source;
    };

    std::unordered_map<std::string, Pair> resources;

    // go through all files, create meta if not exists
    for (const auto& entry : fs::recursive_directory_iterator(assets_root)) {
        if (entry.is_regular_file()) {
            fs::path relative = fs::relative(entry.path(), assets_root);
            std::string short_path = std::format("@res://{}", relative.generic_string());

            auto ext = StringUtils::Extension(short_path);
            if (ext == ".meta") {
                short_path.resize(short_path.size() - 5);  // remove '.meta'
                resources[short_path].has_meta = true;
            } else {
                resources[short_path].has_source = true;
            }
        }
    }

    std::vector<AssetMetaData> assets;
    assets.reserve(resources.size());

    for (const auto& [key, value] : resources) {
        auto meta_path = std::format("{}.meta", key);
        if (value.has_meta) {
            auto res = AssetMetaData::LoadMeta(meta_path);
            if (!res) {
                return HBN_ERROR(res.error());
            }

            LOG_VERBOSE("'{}' detected, loading...", meta_path);
            assets.emplace_back(std::move(*res));
            continue;
        }

        DEV_ASSERT(value.has_source);
        auto meta = AssetMetaData::CreateMeta(key);

        auto res = meta.SaveMeta(meta_path);
        if (!res) {
            return HBN_ERROR(res.error());
        }

        LOG_VERBOSE("'{}' not detected, creating", meta_path);
        assets.emplace_back(std::move(meta));
    }

    std::latch latch(assets.size());
    for (auto& meta : assets) {
        StartAsyncLoad(std::move(meta), [](AssetRef p_asset, void* p_userdata) {
            unused(p_asset);
            DEV_ASSERT(p_userdata);
            std::latch& latch = *reinterpret_cast<std::latch*>(p_userdata);
            latch.count_down(); }, &latch);
    }

    latch.wait();
    return Result<void>();
}

void AssetRegistry::FinalizeImpl() {
    // @TODO: clean up all the stuff
}

AssetHandle AssetRegistry::StartAsyncLoad(AssetMetaData&& p_meta,
                                          OnAssetLoadSuccessFunc p_on_success,
                                          void* p_userdata) {

    auto entry = std::make_shared<AssetEntry>(std::move(p_meta));
    {
        std::lock_guard lock(registry_mutex);
        bool ok = true;
        ok = ok && m_guid_map.try_emplace(entry->metadata.guid, entry).second;
        ok = ok && m_path_map.try_emplace(entry->metadata.path, entry->metadata.guid).second;
    }

    m_app->GetAssetManager()->LoadAssetAsync(entry.get(), p_on_success, p_userdata);
    return { entry->metadata.guid, entry };
}

AssetHandle AssetRegistry::Request(const std::string& p_path) {
    std::lock_guard lock(registry_mutex);
    auto it = m_path_map.find(p_path);
    if (it != m_path_map.end()) {
        const Guid& guid = it->second;
        auto it2 = m_guid_map.find(guid);
        if (it2 != m_guid_map.end()) {
            return AssetHandle{ guid, it2->second };
        }
    }

    return AssetHandle{ .guid = Guid(), .entry = nullptr };
}

#if 0
void AssetRegistry::GetAssetByType(AssetType p_type, std::vector<IAsset*>& p_out) {
    std::lock_guard gurad(m_lock);
    for (auto& it : m_handles) {
        if (it->asset && it->asset->type == p_type) {
            p_out.emplace_back(it->asset.get());
        }
    }
}

void AssetRegistry::RemoveAsset(const std::string& p_path) {
    std::lock_guard gurad(m_lock);
    auto it = m_lookup.find(p_path);
    if (it == m_lookup.end()) {
        return;
    }

    auto val = it->second;
    m_lookup.erase(it);
    for (auto it2 = m_handles.begin(); it2 != m_handles.end(); ++it2) {
        if (it2->get() == val) {
            m_handles.erase(it2);
            break;
        }
    }
}
#endif

}  // namespace my
