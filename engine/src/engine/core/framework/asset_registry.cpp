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
        const auto& type_json = meta_json["type"];
        const auto& path_json = meta_json["path"];
        if (path_json.is_string()) {
            if (type_json == "Font") {
                AssetMetaData meta_data;
                meta_data.type = AssetType::BUFFER;
                meta_data.path = path_json;
                if (!meta_data.path.empty()) {
                    meta_data.handle = meta_data.path;
                    asset_bundle.emplace_back(std::move(meta_data));
                }
            }
        }
    }

    RegisterAssets((int)asset_bundle.size(), asset_bundle.data());

    AssetManager* asset_manager = m_app->GetAssetManager();
    for (auto& [key, value] : m_lookup) {
        AssetStub* stub = &value;
        asset_manager->LoadAssetAsync(
            value.meta, [](void* p_asset, void* p_userdata) {
                AssetStub* stub = reinterpret_cast<AssetStub*>(p_userdata);
                stub->asset = reinterpret_cast<IAsset*>(p_asset);
                stub->state = AssetStub::ASSET_STATE_READY;
            },
            stub);
    }

    return Result<void>();
}

void AssetRegistry::FinalizeImpl() {
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

        auto& stub = m_lookup[handle];
        stub.state = AssetStub::ASSET_STATE_LOADING;
        stub.meta = std::move(p_metas[i]);
        stub.asset = nullptr;
    }
}

const IAsset* AssetRegistry::GetAssetByHandle(const AssetHandle& p_handle) {
    std::lock_guard gurad(m_lock);
    auto it = m_lookup.find(p_handle);
    if (it == m_lookup.end()) {
        return nullptr;
    }

    return it->second.asset;
}

}  // namespace my
