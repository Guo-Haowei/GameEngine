#pragma once
#include "engine/assets/asset_interface.h"
#include "engine/core/base/concurrent_queue.h"
#include "engine/core/base/singleton.h"
#include "engine/core/io/file_path.h"
#include "engine/runtime/module.h"

namespace my {

class AssetEntry;
class Scene;

class AssetManager : public Singleton<AssetManager>, public Module {
public:
    struct LoadTask;

    AssetManager()
        : Module("AssetManager") {}

    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    auto LoadFileSync(const FilePath& p_path) -> Result<AssetRef>;

    static void WorkerMain();
    static void RequestShutdown();

private:
    [[nodiscard]] auto LoadAssetSync(const AssetEntry* p_entry) -> Result<AssetRef>;
    void LoadAssetAsync(AssetEntry* p_entry,
                        OnAssetLoadSuccessFunc p_on_success,
                        void* p_userdata);

    void EnqueueLoadTask(LoadTask& p_task);

    std::mutex m_assetLock;

    friend class AssetRegistry;
};

}  // namespace my
