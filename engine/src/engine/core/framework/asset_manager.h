#pragma once
#include "engine/assets/asset.h"
#include "engine/core/base/concurrent_queue.h"
#include "engine/core/base/singleton.h"
#include "engine/core/framework/module.h"
#include "engine/core/io/file_path.h"

namespace my {

class Scene;
struct LoadTask;
struct AssetRegistryHandle;

using LoadSuccessFunc = void (*)(void* p_asset, void* p_userdata);

class AssetManager : public Singleton<AssetManager>, public Module {
public:
    AssetManager() : Module("AssetManager") {}

    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    void LoadSceneAsync(const FilePath& p_path, LoadSuccessFunc p_on_success);

    auto LoadFileSync(const FilePath& p_path) -> Result<std::shared_ptr<File>>;
    std::shared_ptr<File> FindFile(const FilePath& p_path);

    static void WorkerMain();
    static void Wait();

private:
    void LoadAssetAsync(AssetRegistryHandle* p_handle,
                        LoadSuccessFunc p_on_success = nullptr,
                        void* p_user_data = nullptr);

    void EnqueueLoadTask(LoadTask& p_task);

    std::map<FilePath, std::shared_ptr<File>> m_textCache;

    friend class AssetRegistry;
};

}  // namespace my
