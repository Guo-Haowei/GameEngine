#pragma once
#include "engine/assets/asset.h"
#include "engine/core/base/singleton.h"
#include "engine/core/framework/module.h"
#include "engine/core/io/file_path.h"

namespace my {

struct AssetStub {
    enum State : uint8_t {
        ASSET_STATE_LOADING,
        ASSET_STATE_READY,
    };

    std::atomic<State> state;

    // loading?
    // loaded?
    // unloading?
    AssetMetaData meta;
    IAsset* asset;
};

class AssetRegistry : public Singleton<AssetRegistry>, public Module {
public:
    AssetRegistry() : Module("AssetRegistry") {}

    void RegisterAssets(int p_count, AssetMetaData* p_metas);

    const IAsset* GetAssetByHandle(const AssetHandle& p_handle);

    // virtual void SearchAllAssets(bool bIncludeSubFolders) = 0;
    // virtual bool GetAssetByObjectPath(FName ObjectPath, FAssetData& OutAssetData) const = 0;
    // virtual bool GetAssetsByClass(FName ClassName, TArray<FAssetData>& OutAssetData) const = 0;
    // virtual bool GetAssetsByTag(FName TagName, TArray<FAssetData>& OutAssetData) const = 0;

protected:
    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    std::unordered_map<AssetHandle, AssetStub> m_lookup;
    std::mutex m_lock;
};

}  // namespace my
