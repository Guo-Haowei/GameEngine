#pragma once
#include "engine/assets/asset.h"
#include "engine/core/base/singleton.h"
#include "engine/core/framework/module.h"
#include "engine/core/io/file_path.h"

namespace my {

using LoadSuccessFunc = void (*)(void* p_asset, void* p_userdata);

struct AssetRegistryHandle {
    enum State : uint8_t {
        ASSET_STATE_LOADING,
        ASSET_STATE_READY,
    };

    AssetRegistryHandle(const IAsset::Meta& p_meta) : meta(p_meta),
                                                      asset(nullptr),
                                                      state(ASSET_STATE_LOADING) {}

    IAsset::Meta meta;
    IAsset* asset;
    std::atomic<State> state;
};

class AssetRegistry : public Singleton<AssetRegistry>, public Module {
    enum RequestMode {
        LOAD_ASYNC,
        LOAD_SYNC,
    };

public:
    AssetRegistry() : Module("AssetRegistry") {}

    const IAsset* GetAssetByHandle(const AssetHandle& p_handle);

    void GetAssetByType(AssetType p_type, std::vector<IAsset*>& p_out);

    template<typename T>
    const T* GetAssetByHandle(const AssetHandle& p_handle) {
        const IAsset* tmp = GetAssetByHandle(p_handle);
        if (tmp) {
            const T* asset = dynamic_cast<const T*>(tmp);
            if (DEV_VERIFY(asset)) {
                return asset;
            }
        }
        return nullptr;
    }

    void RegisterAssets(int p_count, IAsset::Meta* p_metas);

    auto RequestAssetSync(const std::string& p_path) {
        return RequestAssetImpl(p_path, LOAD_SYNC, nullptr, nullptr);
    }

    auto RequestAssetAsync(const std::string& p_path,
                           LoadSuccessFunc p_on_success = nullptr,
                           void* p_user_data = nullptr) {
        return RequestAssetImpl(p_path, LOAD_ASYNC, p_on_success, p_user_data);
    }

protected:
    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    auto RequestAssetImpl(const std::string& p_path,
                          RequestMode p_mode,
                          LoadSuccessFunc p_on_success,
                          void* p_user_data) -> Result<const IAsset*>;

    std::unordered_map<AssetHandle, AssetRegistryHandle*> m_lookup;
    std::vector<std::unique_ptr<AssetRegistryHandle>> m_handles;
    std::mutex m_lock;
};

}  // namespace my
