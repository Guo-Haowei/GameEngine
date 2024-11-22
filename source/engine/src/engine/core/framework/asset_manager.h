#pragma once
#include "assets/image.h"
#include "core/base/concurrent_queue.h"
#include "core/base/singleton.h"
#include "core/framework/module.h"
#include "core/io/file_path.h"

namespace my {

// @TODO: refactor
struct File {
    std::vector<char> buffer;
};

class Scene;

using LoadSuccessFunc = void (*)(void* p_asset, void* p_userdata);

enum LoadTaskType {
    LOAD_TASK_IMAGE,
    LOAD_TASK_SCENE,
};

struct LoadTask {
    LoadTaskType type;
    FilePath assetPath;
    LoadSuccessFunc onSuccess;
    void* userdata;
};

class AssetManager : public Singleton<AssetManager>, public Module {
public:
    AssetManager() : Module("AssetManager") {}

    auto Initialize() -> Result<void> override;
    void Finalize() override;

    void LoadSceneAsync(const FilePath& p_path, LoadSuccessFunc p_on_success);

    ImageHandle* LoadImageSync(const FilePath& p_path);
    ImageHandle* LoadImageAsync(const FilePath& p_path, LoadSuccessFunc = nullptr);
    ImageHandle* FindImage(const FilePath& p_path);

    std::shared_ptr<File> LoadFileSync(const FilePath& p_path);
    std::shared_ptr<File> FindFile(const FilePath& p_path);

    static void WorkerMain();

private:
    void EnqueueLoadTask(LoadTask& p_task);

    std::map<FilePath, std::unique_ptr<ImageHandle>> m_imageCache;
    std::map<FilePath, std::shared_ptr<File>> m_textCache;
    std::mutex m_imageCacheLock;
};

}  // namespace my
