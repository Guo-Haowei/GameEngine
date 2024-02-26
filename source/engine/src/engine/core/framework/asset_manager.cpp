#include "asset_manager.h"

#include "assets/loader_assimp.h"
#include "assets/loader_stbi.h"
#include "assets/loader_tinygltf.h"
#include "core/framework/graphics_manager.h"
#include "core/io/file_access.h"
#include "core/os/threads.h"
#include "core/os/timer.h"
#include "scene/scene.h"

namespace my {

static struct {
    // @TODO: better wake up
    std::condition_variable wake_condition;
    std::mutex wake_mutex;
    // @TODO: better thread safe queue
    ConcurrentQueue<LoadTask> job_queue;
    // file
    std::map<std::string, std::shared_ptr<File>> text_cache;
} s_asset_manager_glob;

bool AssetManager::initialize() {
    Loader<Scene>::register_loader(".obj", LoaderAssimp::create);
    Loader<Scene>::register_loader(".gltf", LoaderTinyGLTF::create);

    Loader<Image>::register_loader(".png", LoaderSTBI8::create);
    Loader<Image>::register_loader(".jpg", LoaderSTBI8::create);
    Loader<Image>::register_loader(".hdr", LoaderSTBI32::create);

    // @TODO: dir_access
    // @TODO: async
    // force load all shaders
#if 0
    const std::string preload[] = {
        "@res://fonts/DroidSans.ttf",
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
    s_asset_manager_glob.wake_condition.notify_all();
}

void AssetManager::enqueue_async_load_task(LoadTask& task) {
    s_asset_manager_glob.job_queue.push(std::move(task));
    s_asset_manager_glob.wake_condition.notify_one();
}

ImageHandle* AssetManager::find_image(const std::string& p_path) {
    std::lock_guard guard(m_image_cache_lock);

    auto found = m_image_cache.find(p_path);
    if (found != m_image_cache.end()) {
        return found->second.get();
    }

    return nullptr;
}

ImageHandle* AssetManager::load_image_async(const std::string& p_path, LoadSuccessFunc p_on_success) {
    m_image_cache_lock.lock();

    auto found = m_image_cache.find(p_path);
    if (found != m_image_cache.end()) {
        auto ret = found->second.get();
        m_image_cache_lock.unlock();
        return ret;
    }

    auto handle = std::make_unique<AssetHandle<Image>>();
    handle->state = ASSET_STATE_LOADING;
    ImageHandle* ret = handle.get();
    m_image_cache[p_path] = std::move(handle);
    m_image_cache_lock.unlock();

    LoadTask task;
    task.type = LOAD_TASK_IMAGE;
    if (p_on_success) {
        task.on_success = p_on_success;
    } else {
        task.on_success = [](void* p_asset, void* p_userdata) {
            Image* image = reinterpret_cast<Image*>(p_asset);
            ImageHandle* handle = reinterpret_cast<ImageHandle*>(p_userdata);
            DEV_ASSERT(image);
            DEV_ASSERT(handle);

            handle->set(image);
            GraphicsManager::singleton().request_texture(handle);
        };
    }
    task.userdata = ret;
    task.asset_path = p_path;
    enqueue_async_load_task(task);
    return ret;
}

ImageHandle* AssetManager::load_image_sync(const std::string& p_path) {
    std::lock_guard guard(m_image_cache_lock);

    auto found = m_image_cache.find(p_path);
    if (found != m_image_cache.end()) {
        DEV_ASSERT(found->second->state.load() == ASSET_STATE_READY);
        return found->second.get();
    }

    // LOG_VERBOSE("image {} not found in cache, loading...", path);
    auto handle = std::make_unique<AssetHandle<Image>>();
    auto loader = Loader<Image>::create(p_path);
    if (!loader) {
        return nullptr;
    }

    Image* image = new Image(p_path);
    if (!loader->load(image)) {
        delete image;
        return nullptr;
    }
    handle->set(image);
    ImageHandle* ret = handle.get();
    m_image_cache[p_path] = std::move(handle);
    GraphicsManager::singleton().create_texture(ret);
    return ret;
}

void AssetManager::load_scene_async(const std::string& p_path, LoadSuccessFunc p_on_success) {
    LoadTask task;
    task.type = LOAD_TASK_SCENE;
    task.asset_path = p_path;
    task.on_success = p_on_success;
    task.userdata = nullptr;
    enqueue_async_load_task(task);
}

template<typename T>
static void load_asset(LoadTask& p_task, T* p_asset) {
    auto loader = Loader<T>::create(p_task.asset_path);
    if (!loader) {
        LOG_ERROR("[AssetManager] not loader found for '{}'", p_task.asset_path);
        return;
    }

    Timer timer;
    if (loader->load(p_asset)) {
        p_task.on_success(p_asset, p_task.userdata);
        LOG_VERBOSE("[AssetManager] asset '{}' loaded in {}", p_task.asset_path, timer.get_duration_string());
    } else {
        LOG_ERROR("[AssetManager] failed to load '{}', details: {}", p_task.asset_path, loader->get_error());
    }
}

void AssetManager::worker_main() {
    for (;;) {
        if (thread::is_shutdown_requested()) {
            break;
        }

        LoadTask task;
        if (!s_asset_manager_glob.job_queue.pop(task)) {
            std::unique_lock<std::mutex> lock(s_asset_manager_glob.wake_mutex);
            s_asset_manager_glob.wake_condition.wait(lock);
            continue;
        }

        // LOG_VERBOSE("[AssetManager] start loading asset '{}'", task.asset_path);
        switch (task.type) {
            case LOAD_TASK_IMAGE: {
                load_asset<Image>(task, new Image(task.asset_path));
            } break;
            case LOAD_TASK_SCENE: {
                LOG_VERBOSE("[AssetManager] start loading scene {}", task.asset_path);
                load_asset<Scene>(task, new Scene);
            } break;
            default:
                CRASH_NOW();
                break;
        }
    }
}

std::shared_ptr<File> AssetManager::find_file(const std::string& p_path) {
    auto found = s_asset_manager_glob.text_cache.find(p_path);
    if (found != s_asset_manager_glob.text_cache.end()) {
        return found->second;
    }

    return nullptr;
}

std::shared_ptr<File> AssetManager::load_file_sync(const std::string& p_path) {
    auto found = s_asset_manager_glob.text_cache.find(p_path);
    if (found != s_asset_manager_glob.text_cache.end()) {
        return found->second;
    }

    auto res = FileAccess::open(p_path, FileAccess::READ);
    if (!res) {
        LOG_ERROR("[FileAccess] Error: failed to open file '{}', reason: {}", p_path, res.error().get_message());
        return nullptr;
    }

    std::shared_ptr<FileAccess> file_access = *res;

    const size_t size = file_access->get_length();

    std::vector<char> buffer;
    buffer.resize(size);
    file_access->read_buffer(buffer.data(), size);
    auto text = std::make_shared<File>();
    text->buffer = std::move(buffer);
    s_asset_manager_glob.text_cache[p_path] = text;
    return text;
}

}  // namespace my
