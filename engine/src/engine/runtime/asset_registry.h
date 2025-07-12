#pragma once
#include "engine/assets/asset.h"
#include "engine/assets/asset_entry.h"
#include "engine/assets/asset_handle.h"
#include "engine/core/base/singleton.h"
#include "engine/core/io/file_path.h"
#include "engine/runtime/module.h"

namespace my {

class AssetRegistry : public Singleton<AssetRegistry>, public Module {
public:
    AssetRegistry()
        : Module("AssetRegistry") {}

    AssetHandle Request(const std::string& p_path,
                        OnAssetLoadSuccessFunc p_on_success = nullptr,
                        void* p_userdata = nullptr);

#if 0
    void GetAssetByType(AssetType p_type, std::vector<IAsset*>& p_out);
    void RemoveAsset(const std::string& p_path);
#endif

protected:
    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    AssetHandle Request(AssetMetaData&& p_meta,
                        OnAssetLoadSuccessFunc p_on_success,
                        void* p_userdata);

    mutable std::mutex registry_mutex;
    std::unordered_map<std::string, std::shared_ptr<AssetEntry>> path_map;
    std::unordered_map<Guid, std::shared_ptr<AssetEntry>> uuid_map;
};

}  // namespace my
