#pragma once
#include "engine/assets/asset.h"
#include "engine/core/base/singleton.h"
#include "engine/core/framework/module.h"
#include "engine/core/io/file_path.h"

namespace my {

struct AssetRegistryHandle {
    enum State : uint8_t {
        ASSET_STATE_LOADING,
        ASSET_STATE_READY,
    };

    AssetRegistryHandle(const AssetMetaData& p_meta) : meta(p_meta),
                                                       asset(nullptr),
                                                       state(ASSET_STATE_LOADING) {}

    AssetMetaData meta;
    IAsset* asset;
    std::atomic<State> state;
};

class AssetRegistry : public Singleton<AssetRegistry>, public Module {
public:
    AssetRegistry() : Module("AssetRegistry") {}

    const IAsset* GetAssetByHandle(const AssetHandle& p_handle);

    const IAsset* RequestAsset(const std::string& p_path);

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

    void RegisterAssets(int p_count, AssetMetaData* p_metas);

protected:
    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    std::unordered_map<AssetHandle, AssetRegistryHandle*> m_lookup;
    std::vector<std::unique_ptr<AssetRegistryHandle>> m_handles;
    std::mutex m_lock;
};

}  // namespace my
