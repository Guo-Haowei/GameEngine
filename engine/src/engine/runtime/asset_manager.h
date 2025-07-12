#pragma once
#include "engine/assets/asset_interface.h"
#include "engine/core/base/concurrent_queue.h"
#include "engine/core/base/singleton.h"
#include "engine/runtime/module.h"

namespace my {

class AssetEntry;
class AssetType;
class Scene;

class AssetManager : public Singleton<AssetManager>, public Module {
public:
    struct LoadTask;

    AssetManager()
        : Module("AssetManager") {}

    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    void CreateAsset(const AssetType& p_type, const std::filesystem::path& p_folder, const char* p_name = nullptr);

    std::string ResolvePath(const std::filesystem::path& p_path);

    static void WorkerMain();
    static void RequestShutdown();

private:
    [[nodiscard]] auto LoadAssetSync(const AssetEntry* p_entry) -> Result<AssetRef>;
    void LoadAssetAsync(AssetEntry* p_entry,
                        OnAssetLoadSuccessFunc p_on_success,
                        void* p_userdata);

    void EnqueueLoadTask(LoadTask& p_task);

    uint32_t m_counter{ 0 };
    std::mutex m_assetLock;
    std::filesystem::path m_assets_root;

    friend class AssetRegistry;
};

}  // namespace my
