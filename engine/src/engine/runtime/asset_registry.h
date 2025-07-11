#pragma once
#include "engine/assets/asset_entry.h"
#include "engine/assets/asset_interface.h"
#include "engine/assets/asset_handle.h"
#include "engine/core/base/singleton.h"
#include "engine/runtime/module.h"

namespace my {

class AssetRegistry : public Singleton<AssetRegistry>, public Module {
public:
    AssetRegistry()
        : Module("AssetRegistry") {}

    AssetHandle Request(const std::string& p_path);

#if 0
    void GetAssetByType(AssetType p_type, std::vector<IAsset*>& p_out);
    void RemoveAsset(const std::string& p_path);
#endif

protected:
    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    bool StartAsyncLoad(AssetMetaData&& p_meta,
                        OnAssetLoadSuccessFunc p_on_success,
                        void* p_userdata);

    mutable std::mutex registry_mutex;
    std::unordered_map<std::string, Guid> m_path_map;
    std::unordered_map<Guid, std::shared_ptr<AssetEntry>> m_guid_map;

    friend class AssetManager;
};

}  // namespace my
