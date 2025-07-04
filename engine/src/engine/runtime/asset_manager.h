#pragma once
#include "engine/assets/asset.h"
#include "engine/core/base/concurrent_queue.h"
#include "engine/core/base/singleton.h"
#include "engine/core/io/file_path.h"
#include "engine/runtime/module.h"

namespace my {

class Scene;
struct IAsset;
struct LoadTask;
struct AssetRegistryHandle;

class AssetManager : public Singleton<AssetManager>, public Module {
public:
    AssetManager() : Module("AssetManager") {}

    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    auto LoadFileSync(const FilePath& p_path) -> Result<std::shared_ptr<IAsset>>;

    static void WorkerMain();
    static void Wait();
    static void RequestShutdown();

private:
    [[nodiscard]] auto LoadAssetSync(AssetRegistryHandle* p_handle) -> Result<IAsset*>;

    void LoadAssetAsync(AssetRegistryHandle* p_handle,
                        OnAssetLoadSuccessFunc p_on_success,
                        void* p_user_data);

    void EnqueueLoadTask(LoadTask& p_task);

    std::mutex m_assetLock;
    std::vector<std::unique_ptr<IAsset>> m_assets;

    friend class AssetRegistry;
};

}  // namespace my
