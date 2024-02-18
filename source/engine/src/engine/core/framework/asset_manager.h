#pragma once
#include "assets/image.h"
#include "assets/scene_importer.h"
#include "core/base/concurrent_queue.h"
#include "core/base/singleton.h"
#include "core/framework/module.h"

namespace my {

// @TODO: refactor
struct File {
    std::vector<char> buffer;
};

class Scene;

class AssetManager : public Singleton<AssetManager>, public Module {
public:
    AssetManager() : Module("AssetManager") {}

    bool initialize() override;
    void finalize() override;
    void update();

    ImageHandle* load_image_sync(const std::string& path);
    ImageHandle* load_image_async(const std::string& path);
    ImageHandle* find_image(const std::string& path);

private:
    std::map<std::string, std::unique_ptr<ImageHandle>> m_image_cache;
    std::mutex m_image_cache_lock;
};

}  // namespace my

namespace my::asset_loader {

using ImportSuccessFunc = void (*)(void*);
using ImportErrorFunc = void (*)(const std::string& error);

void load_scene_async(ImporterName importer, const std::string& path, ImportSuccessFunc on_success, ImportErrorFunc on_error = nullptr);

std::shared_ptr<File> load_file_sync(const std::string& path);
std::shared_ptr<File> find_file(const std::string& path);

void worker_main();

}  // namespace my::asset_loader
