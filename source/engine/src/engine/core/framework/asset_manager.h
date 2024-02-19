#pragma once
#include "assets/image.h"
#include "core/base/concurrent_queue.h"
#include "core/base/singleton.h"
#include "core/framework/module.h"

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
    // @TODO: better string
    std::string asset_path;
    LoadSuccessFunc on_success;
    void* userdata;
};

class AssetManager : public Singleton<AssetManager>, public Module {
public:
    AssetManager() : Module("AssetManager") {}

    bool initialize() override;
    void finalize() override;
    void update();

    void load_scene_async(const std::string& path, LoadSuccessFunc on_success);

    ImageHandle* load_image_sync(const std::string& path);
    ImageHandle* load_image_async(const std::string& path);
    ImageHandle* find_image(const std::string& path);

    std::shared_ptr<File> load_file_sync(const std::string& path);
    std::shared_ptr<File> find_file(const std::string& path);

    static void worker_main();

private:
    void enqueue_async_load_task(LoadTask& task);

    std::map<std::string, std::unique_ptr<ImageHandle>> m_image_cache;
    std::mutex m_image_cache_lock;
};

}  // namespace my
