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

using LoadSuccessFunc = void (*)(void* asset, void* userdata);

enum LoadTaskType {
    LOAD_TASK_IMAGE,
    LOAD_TASK_SCENE,
};

struct LoadTask {
    LoadTaskType type;
    FilePath asset_path;
    LoadSuccessFunc on_success;
    void* userdata;
};

class AssetManager : public Singleton<AssetManager>, public Module {
public:
    AssetManager() : Module("AssetManager") {}

    bool Initialize() override;
    void Finalize() override;

    void loadSceneAsync(const FilePath& p_path, LoadSuccessFunc p_on_success);

    ImageHandle* loadImageSync(const FilePath& p_path);
    ImageHandle* loadImageAsync(const FilePath& p_path, LoadSuccessFunc = nullptr);
    ImageHandle* findImage(const FilePath& p_path);

    std::shared_ptr<File> loadFileSync(const FilePath& p_path);
    std::shared_ptr<File> findFile(const FilePath& p_path);

    static void workerMain();

private:
    void enqueueLoadTask(LoadTask& task);

    std::map<FilePath, std::unique_ptr<ImageHandle>> m_image_cache;
    std::map<FilePath, std::shared_ptr<File>> m_text_cache;
    std::mutex m_image_cache_lock;
};

}  // namespace my
