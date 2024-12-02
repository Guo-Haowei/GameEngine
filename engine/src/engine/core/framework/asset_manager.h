#pragma once
#include "engine/assets/asset.h"
#include "engine/assets/image.h"
#include "engine/core/base/concurrent_queue.h"
#include "engine/core/base/singleton.h"
#include "engine/core/framework/module.h"
#include "engine/core/io/file_path.h"

namespace my {

// @TODO: refactor
struct File : IAsset {
    std::vector<char> buffer;
};

class Scene;

using LoadSuccessFunc = void (*)(void* p_asset, void* p_userdata);

struct LoadTask;

class AssetManager : public Singleton<AssetManager>, public Module {
public:
    AssetManager() : Module("AssetManager") {}

    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    void LoadSceneAsync(const FilePath& p_path, LoadSuccessFunc p_on_success);

    ImageHandle* LoadImageSync(const FilePath& p_path);
    ImageHandle* LoadImageAsync(const FilePath& p_path, LoadSuccessFunc = nullptr);
    ImageHandle* FindImage(const FilePath& p_path);

    auto LoadFileSync(const FilePath& p_path) -> Result<std::shared_ptr<File>>;
    std::shared_ptr<File> FindFile(const FilePath& p_path);

    static void WorkerMain();
    static void Wait();

private:
    void LoadAssetAsync(const AssetMetaData& p_meta, LoadSuccessFunc p_on_success, void* p_user_data);

    void EnqueueLoadTask(LoadTask& p_task);

    std::map<FilePath, std::unique_ptr<ImageHandle>> m_imageCache;
    std::map<FilePath, std::shared_ptr<File>> m_textCache;
    std::mutex m_imageCacheLock;

    friend class AssetRegistry;
};

}  // namespace my
