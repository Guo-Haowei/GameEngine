#include "asset_registry.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "engine/core/framework/application.h"
#include "engine/core/framework/asset_manager.h"

namespace my {

namespace fs = std::filesystem;

auto AssetRegistry::InitializeImpl() -> Result<void> {
    // Always load assets
    fs::path assets_root = fs::path{ m_app->GetResourceFolder() };
    fs::path always_load = assets_root / "alwaysload.json";
    std::ifstream file(always_load);
    using json = nlohmann::json;
    json data = json::parse(file);

    DEV_ASSERT(data.is_array());

    std::vector<AssetMetaData> asset_bundle;

    for (const auto& meta_json : data) {
        // const auto& type_json = meta_json["type"];
        const auto& path_json = meta_json["path"];
        if (path_json.is_string()) {
            AssetMetaData meta_data;
            meta_data.path = path_json;
            if (!meta_data.path.empty()) {
                meta_data.handle = meta_data.path;
                asset_bundle.emplace_back(std::move(meta_data));
            }
        }
    }

    RegisterAssets((int)asset_bundle.size(), asset_bundle.data());

    AssetManager* asset_manager = m_app->GetAssetManager();
    for (auto& [key, stub] : m_lookup) {
        asset_manager->LoadAssetAsync(stub);
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

const IAsset* AssetRegistry::RequestAsset(const std::string& p_path) {
    AssetMetaData meta;
    meta.handle = p_path;
    meta.path = p_path;

    std::lock_guard gurad(m_lock);
    auto it = m_lookup.find(meta.handle);
    if (it != m_lookup.end()) {
        return it->second->asset;
    }

    auto stub = new AssetRegistryHandle(std::move(meta));

    m_lookup[p_path] = stub;
    m_handles.emplace_back(std::unique_ptr<AssetRegistryHandle>(stub));
    return stub->asset;
}

void AssetRegistry::RegisterAssets(int p_count, AssetMetaData* p_metas) {
    DEV_ASSERT(p_count);

    std::lock_guard gurad(m_lock);
    for (int i = 0; i < p_count; ++i) {
        auto handle = p_metas[i].handle;
        DEV_ASSERT(!handle.empty());
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
