#include "asset_registry.h"

#include <yaml-cpp/yaml.h>

#include <fstream>

#include "engine/runtime/application.h"
#include "engine/runtime/asset_manager.h"

namespace my {

namespace fs = std::filesystem;

auto AssetRegistry::InitializeImpl() -> Result<void> {
    // @TODO: refactor
    // Always load assets
    fs::path assets_root = fs::path{ m_app->GetResourceFolder() };
    fs::path always_load_path = assets_root / "alwaysload.yaml";
    std::ifstream file(always_load_path);
    if (file) {
        YAML::Node node = YAML::Load(file);

        const auto& files = node["files"];
        if (!files.IsSequence()) {
            return HBN_ERROR(ErrorCode::ERR_PARSE_ERROR, "failed to parse {}, error: 'files' not found", always_load_path.string());
        }

        std::vector<IAsset::Meta> asset_bundle;

        for (const auto& item : files) {
            const auto& path = item["path"];
            if (!path.IsScalar()) {
                CRASH_NOW();
            }

            IAsset::Meta meta_data;
            meta_data.path = path.as<std::string>();
            if (!meta_data.path.empty()) {
                meta_data.handle = meta_data.path;
                asset_bundle.emplace_back(std::move(meta_data));
            }
        }

        RegisterAssets((int)asset_bundle.size(), asset_bundle.data());

        AssetManager* asset_manager = m_app->GetAssetManager();
        for (auto& [key, stub] : m_lookup) {
            if (auto res = asset_manager->LoadAssetSync(stub); !res) {
                return HBN_ERROR(res.error());
            }
        }
    }
    return Result<void>();
}

void AssetRegistry::FinalizeImpl() {
}

const IAsset* AssetRegistry::GetAssetByHandle(const AssetHandle& p_handle) {
    std::lock_guard gurad(m_lock);
    auto it = m_lookup.find(p_handle);
    if (it == m_lookup.end()) {
        return nullptr;
    }

    return it->second->asset;
}

void AssetRegistry::GetAssetByType(AssetType p_type, std::vector<IAsset*>& p_out) {
    std::lock_guard gurad(m_lock);
    for (auto& it : m_handles) {
        if (it->asset && it->asset->type == p_type) {
            p_out.emplace_back(it->asset);
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

auto AssetRegistry::RequestAssetImpl(const std::string& p_path,
                                     RequestMode p_mode,
                                     OnAssetLoadSuccessFunc p_on_success,
                                     void* p_user_data) -> Result<const IAsset*> {
    IAsset::Meta meta;
    meta.handle = p_path;
    meta.path = p_path;

    std::lock_guard gurad(m_lock);
    auto it = m_lookup.find(meta.handle);
    if (it != m_lookup.end()) {
#if USING(DEBUG_BUILD)
        if (it->first.path != p_path) {
            auto error = std::format("hash collision '{}' and '{}'", p_path, it->first.path);
            CRASH_NOW_MSG(error);
        }
#endif
        return it->second->asset;
    }

    auto stub = new AssetRegistryHandle(std::move(meta));

    m_lookup[p_path] = stub;
    m_handles.emplace_back(std::unique_ptr<AssetRegistryHandle>(stub));

    switch (p_mode) {
        case AssetRegistry::LOAD_ASYNC: {
            m_app->GetAssetManager()->LoadAssetAsync(stub, p_on_success, p_user_data);
        } break;
        case AssetRegistry::LOAD_SYNC: {
            auto res = m_app->GetAssetManager()->LoadAssetSync(stub);
            if (!res) {
                return HBN_ERROR(res.error());
            }
        } break;
        default:
            CRASH_NOW();
            break;
    }

    return stub->asset;
}

void AssetRegistry::RegisterAssets(int p_count, IAsset::Meta* p_metas) {
    DEV_ASSERT(p_count);

    std::lock_guard gurad(m_lock);
    for (int i = 0; i < p_count; ++i) {
        auto handle = p_metas[i].handle;
        DEV_ASSERT(handle.hash);
        auto it = m_lookup.find(handle);
        if (it != m_lookup.end()) {
            CRASH_NOW();
            return;
        }

        auto stub = new AssetRegistryHandle(std::move(p_metas[i]));

        m_lookup[handle] = stub;
        m_handles.emplace_back(std::unique_ptr<AssetRegistryHandle>(stub));
    }
}

}  // namespace my
