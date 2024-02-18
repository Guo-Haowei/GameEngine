#include "asset_manager.h"

#include "assets/image_loader.h"
#include "assets/scene_importer_assimp.h"
#include "assets/scene_importer_tinygltf.h"
#include "core/framework/graphics_manager.h"
#include "core/io/file_access.h"
#include "core/os/threads.h"
#include "core/os/timer.h"
#include "scene/scene.h"

using LoadFunc = std::expected<void, std::string> (*)(const std::string& asset_path, void* asset);

using namespace my;
using namespace my::asset_loader;

enum LoadTaskType {
    LOAD_TASK_ASSIMP_SCENE,
    LOAD_TASK_TINYGLTF_SCENE,
    LOAD_TASK_IMAGE,
};

struct LoadTask {
    LoadTaskType type;
    // @TODO: better string
    std::string asset_path;
    ImportSuccessFunc on_success;
    ImportErrorFunc on_error;
    void* userdata;
};

static struct {
    // @TODO: better wake up
    std::condition_variable wake_condition;
    std::mutex wake_mutex;
    // @TODO: better thread safe queue
    ConcurrentQueue<LoadTask> job_queue;
    // file
    std::map<std::string, std::shared_ptr<File>> text_cache;

    // @TODO: refactor
    ConcurrentQueue<ImageHandle*> loaded_images;
} s_glob;

namespace my::asset_loader {

void load_scene_async(ImporterName importer, const std::string& path, ImportSuccessFunc on_success, ImportErrorFunc on_error) {
    auto type = importer == IMPORTER_TINYGLTF ? LOAD_TASK_TINYGLTF_SCENE : LOAD_TASK_ASSIMP_SCENE;
    s_glob.job_queue.push({ type, path, on_success, on_error });
    s_glob.wake_condition.notify_one();
}
std::shared_ptr<File> find_file(const std::string& path) {
    auto found = s_glob.text_cache.find(path);
    if (found != s_glob.text_cache.end()) {
        return found->second;
    }

    return nullptr;
}

std::shared_ptr<File> load_file_sync(const std::string& path) {
    auto found = s_glob.text_cache.find(path);
    if (found != s_glob.text_cache.end()) {
        return found->second;
    }

    auto res = FileAccess::open(path, FileAccess::READ);
    if (!res) {
        LOG_ERROR("[FileAccess] Error: failed to open file '{}', reason: {}", path, res.error().get_message());
        return nullptr;
    }

    std::shared_ptr<FileAccess> file_access = *res;

    const size_t size = file_access->get_length();

    std::vector<char> buffer;
    buffer.resize(size);
    file_access->read_buffer(buffer.data(), size);
    auto text = std::make_shared<File>();
    text->buffer = std::move(buffer);
    s_glob.text_cache[path] = text;
    return text;
}

static void load_image_internal(const std::string& path, ImageHandle* handle) {
    handle->data = load_image(path);
    handle->state = ASSET_STATE_READY;
    s_glob.loaded_images.push(handle);
}

static void load_image_internal(LoadTask& task) {
    load_image_internal(task.asset_path, (ImageHandle*)task.userdata);
}

static void load_scene_internal(LoadTask& task) {
    LOG("[asset_loader] Loading scene '{}'...", task.asset_path);

    Scene* scene = new Scene;
    std::expected<void, std::string> res;

    Timer timer;
    if (task.type == LOAD_TASK_TINYGLTF_SCENE) {
        SceneImporterTinyGLTF loader(*scene, task.asset_path);
        res = loader.import();
    } else {
        SceneImporterAssimp loader(*scene, task.asset_path);
        res = loader.import();
    }

    if (!res) {
        std::string error = std::move(res.error());
        if (task.on_error) {
            task.on_error(error);
        } else {
            LOG_FATAL("{}", res.error());
        }
        return;
    }

    LOG("[asset_loader] Scene '{}' loaded in {}", task.asset_path, timer.get_duration_string());
    task.on_success(scene);
}

static bool work() {
    LoadTask task;
    if (!s_glob.job_queue.pop(task)) {
        return false;
    }

    if (task.type == LOAD_TASK_IMAGE) {
        load_image_internal(task);
    } else {
        load_scene_internal(task);
    }

    return true;
}

void worker_main() {
    for (;;) {
        if (thread::is_shutdown_requested()) {
            break;
        }

        if (!work()) {
            std::unique_lock<std::mutex> lock(s_glob.wake_mutex);
            s_glob.wake_condition.wait(lock);
        }
    }
}

}  // namespace my::asset_loader

namespace my {

bool AssetManager::initialize() {
    // @TODO: dir_access
    // @TODO: async
    // force load all shaders
#if 0
    const std::string preload[] = {
        "@res://fonts/DroidSans.ttf",
        "@res://glsl/vsinput.glsl.h",
        "@res://glsl/cbuffer.glsl.h",
        "@res://glsl/common.glsl",
        "@res://glsl/mesh_static.vert",
        "@res://glsl/mesh_animated.vert",
        "@res://glsl/depth_static.vert",
        "@res://glsl/depth_animated.vert",
        "@res://glsl/depth.frag",
        "@res://glsl/fullscreen.vert",
        "@res://glsl/fxaa.frag",
        "@res://glsl/gbuffer.frag",
        "@res://glsl/ssao.frag",
        "@res://glsl/textureCB.glsl",
        "@res://glsl/lighting.frag",
        "@res://glsl/debug/texture.frag",
        "@res://glsl/vxgi/voxelization.vert",
        "@res://glsl/vxgi/voxelization.geom",
        "@res://glsl/vxgi/voxelization.frag",
        "@res://glsl/vxgi/post.comp",
    };

    Timer timer;
    for (int i = 0; i < array_length(preload); ++i) {
        if (load_file_sync(preload[i])) {
            LOG_VERBOSE("[asset_loader] resource '{}' preloaded", preload[i]);
        }
    }
    LOG_VERBOSE("[asset_loader] preloaded {} assets in {}", array_length(preload), timer.get_duration_string());
#endif

    return true;
}

void AssetManager::finalize() {
    s_glob.wake_condition.notify_all();
}

void AssetManager::update() {
    auto loaded_images = s_glob.loaded_images.pop_all();
    while (!loaded_images.empty()) {
        auto image = loaded_images.front();
        loaded_images.pop();
        DEV_ASSERT(image->state == ASSET_STATE_READY);
        GraphicsManager::singleton().create_texture(image);
        // @TODO: create GPU resource
    }
}

ImageHandle* AssetManager::find_image(const std::string& path) {
    std::lock_guard guard(m_image_cache_lock);

    auto found = m_image_cache.find(path);
    if (found != m_image_cache.end()) {
        return found->second.get();
    }

    return nullptr;
}

ImageHandle* AssetManager::load_image_async(const std::string& path) {
    std::lock_guard guard(m_image_cache_lock);

    auto found = m_image_cache.find(path);
    if (found != m_image_cache.end()) {
        LOG_VERBOSE("image {} found in cache", path);
        return found->second.get();
    }

    auto handle = std::make_unique<AssetHandle<Image>>();
    handle->state = ASSET_STATE_LOADING;
    ImageHandle* ret = handle.get();
    m_image_cache[path] = std::move(handle);

    LoadTask task;
    task.type = LOAD_TASK_IMAGE;
    task.on_success = nullptr;
    task.on_error = nullptr;
    task.userdata = ret;
    task.asset_path = path;

    s_glob.job_queue.push(task);
    s_glob.wake_condition.notify_one();
    return ret;
}

ImageHandle* AssetManager::load_image_sync(const std::string& path) {
    std::lock_guard guard(m_image_cache_lock);

    auto found = m_image_cache.find(path);
    if (found != m_image_cache.end()) {
        LOG_VERBOSE("image {} found in cache", path);
        DEV_ASSERT(found->second->state.load() == ASSET_STATE_READY);
        return found->second.get();
    }

    // LOG_VERBOSE("image {} not found in cache, loading...", path);
    auto handle = std::make_unique<AssetHandle<Image>>();
    handle->data = load_image(path);
    handle->state = ASSET_STATE_READY;
    ImageHandle* ret = handle.get();
    m_image_cache[path] = std::move(handle);
    return ret;
}

}  // namespace my
