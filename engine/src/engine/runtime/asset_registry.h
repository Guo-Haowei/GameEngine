#pragma once
#include "engine/assets/asset.h"
#include "engine/assets/asset_entry.h"
#include "engine/core/base/singleton.h"
#include "engine/core/io/file_path.h"
#include "engine/runtime/module.h"

namespace my {

class AssetRegistry : public Singleton<AssetRegistry>, public Module {
    enum RequestMode {
        LOAD_ASYNC,
        LOAD_SYNC,
    };

public:
    AssetRegistry()
        : Module("AssetRegistry") {}

    const IAsset* GetAssetByHandle(const std::string& p_handle);

    void GetAssetByType(AssetType p_type, std::vector<IAsset*>& p_out);

    template<typename T>
    const T* GetAssetByHandle(const std::string& p_handle) {
        const IAsset* tmp = GetAssetByHandle(p_handle);
        if (tmp) {
            const T* asset = dynamic_cast<const T*>(tmp);
            if (DEV_VERIFY(asset)) {
                return asset;
            }
        }
        return nullptr;
    }

    void RegisterAssets(int p_count, AssetMetaData* p_metas);

    // @TODO: request certain type, if no such type, use default loader
    auto RequestAssetSync(const std::string& p_path) {
        return RequestAssetImpl(p_path, LOAD_SYNC, nullptr, nullptr);
    }

    auto RequestAssetAsync(const std::string& p_path,
                           OnAssetLoadSuccessFunc p_on_success = nullptr,
                           void* p_user_data = nullptr) {
        return RequestAssetImpl(p_path, LOAD_ASYNC, p_on_success, p_user_data);
    }

    void RemoveAsset(const std::string& p_path);

protected:
    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    auto RequestAssetImpl(const std::string& p_path,
                          RequestMode p_mode,
                          OnAssetLoadSuccessFunc p_on_success,
                          void* p_user_data) -> Result<const IAsset*>;

    std::unordered_map<std::string, AssetEntry*> m_lookup;
    std::vector<std::unique_ptr<AssetEntry>> m_handles;
    std::mutex m_lock;
};

}  // namespace my
