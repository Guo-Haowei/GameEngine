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

    for (const auto& [key, value] : resources) {
        if (value.has_meta) {
            auto meta_path = std::format("{}.meta", key);
            auto res = AssetMetaData::LoadMeta(meta_path);
            if (!res) {
                return HBN_ERROR(res.error());
            }

            // @TODO: request download
            LOG_VERBOSE("{} detected, loading", meta_path);
            continue;
        }

        DEV_ASSERT(value.has_source);
        auto meta = AssetMetaData::CreateMeta(key);

        // create meta from
        auto res = meta.SaveMeta(key);
        if (!res) {
            return HBN_ERROR(res.error());
        }
        LOG_VERBOSE("meta not detected for asset '{}', creating...", key);
    }

#if 0
    // @TODO: refactor
    // Always load assets
    fs::path always_load_path = assets_root / "alwaysload.yaml";
    std::ifstream file(always_load_path);
    if (file) {
        return Result<void>();
    }

    YAML::Node node = YAML::Load(file);

    const auto& files = node["files"];
    if (!files.IsSequence()) {
        return HBN_ERROR(ErrorCode::ERR_PARSE_ERROR, "failed to parse {}, error: 'files' not found", always_load_path.string());
    }

    // @TODO: instead of always load, parse meta instead

    std::vector<std::string> asset_bundle;
    for (const auto& item : files) {
        const auto& path = item["path"];
        if (!path.IsScalar()) {
            CRASH_NOW();
        }

        auto path_string = path.as<std::string>();
        if (!path_string.empty()) {
            asset_bundle.emplace_back(std::move(path_string));
        }
    }

    std::latch latch(asset_bundle.size());

    for (const auto& path : asset_bundle) {
        Request(path, [](AssetRef p_asset, void* p_userdata) {
            unused(p_asset);
            DEV_ASSERT(p_userdata);
            std::latch& latch = *reinterpret_cast<std::latch*>(p_userdata);
            latch.count_down(); }, &latch);
    }

    latch.wait();
#endif
    return Result<void>();
}

void AssetRegistry::FinalizeImpl() {
    // @TODO: clean up all the stuff
}

AssetHandle AssetRegistry::Request(const std::string& p_path,
                                   OnAssetLoadSuccessFunc p_on_success,
                                   void* p_userdata) {
    // @TODO: normalize path
    {
        std::lock_guard lock(registry_mutex);
        auto it = path_map.find(p_path);
        if (it != path_map.end()) {
            // @TODO: search for meta?
            return AssetHandle{ it->second->metadata.guid, it->second };
        }
    }

    AssetMetaData meta;
    meta.path = p_path;
    meta.guid = Guid::Create();

    auto entry = std::make_shared<AssetEntry>(meta);
    {
        std::lock_guard lock(registry_mutex);
        path_map[meta.path] = entry;
        uuid_map[meta.guid] = entry;
    }

    // @TODO: async load entry

    m_app->GetAssetManager()->LoadAssetAsync(entry.get(), p_on_success, p_userdata);

    return { meta.guid, entry };
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
