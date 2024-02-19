#include "asset_manager.h"

#include "assets/loader_assimp.h"
#include "assets/loader_tinygltf.h"
#include "assets/stb_image.h"
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

    ConcurrentQueue<ImageHandle*> loaded_images;
} s_asset_manager_glob;

bool AssetManager::initialize() {
    Loader<Scene>::register_loader(".obj", LoaderAssimp::create);
    Loader<Scene>::register_loader(".gltf", LoaderTinyGLTF::create);

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
    s_asset_manager_glob.wake_condition.notify_all();
}

void AssetManager::update() {
    auto loaded_images = s_asset_manager_glob.loaded_images.pop_all();
    while (!loaded_images.empty()) {
        auto image = loaded_images.front();
        loaded_images.pop();
        DEV_ASSERT(image->state == ASSET_STATE_READY);
        GraphicsManager::singleton().create_texture(image);
    }
}

void AssetManager::enqueue_async_load_task(LoadTask& task) {
    s_asset_manager_glob.job_queue.push(std::move(task));
    s_asset_manager_glob.wake_condition.notify_one();
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
    m_image_cache_lock.lock();

    auto found = m_image_cache.find(path);
    if (found != m_image_cache.end()) {
        LOG_VERBOSE("image {} found in cache", path);
        auto ret = found->second.get();
        m_image_cache_lock.unlock();
        return ret;
    }

    auto handle = std::make_unique<AssetHandle<Image>>();
    handle->state = ASSET_STATE_LOADING;
    ImageHandle* ret = handle.get();
    m_image_cache[path] = std::move(handle);
    m_image_cache_lock.unlock();

    LoadTask task;
    task.type = LOAD_TASK_IMAGE;
    task.on_success = nullptr;
    task.on_error = nullptr;
    task.userdata = ret;
    task.asset_path = path;
    enqueue_async_load_task(task);
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
    handle->data = load_image_sync_internal(path);
    handle->state = ASSET_STATE_READY;
    ImageHandle* ret = handle.get();
    m_image_cache[path] = std::move(handle);
    GraphicsManager::singleton().create_texture(ret);
    return ret;
}

void AssetManager::load_scene_async(const std::string& path, ImportSuccessFunc on_success, ImportErrorFunc on_error) {
    LoadTask task;
    task.asset_path = path;
    task.on_success = on_success;
    task.on_error = on_error;
    enqueue_async_load_task(task);
}

Image* AssetManager::load_image_sync_internal(const std::string& path) {
    int width = 0;
    int height = 0;
    int num_channels = 0;

    auto res = FileAccess::open(path, FileAccess::READ);
    if (!res) {
        LOG_ERROR("[FileAccess] Error: failed to open file '{}', reason: {}", path, res.error().get_message());
        return nullptr;
    }

    std::shared_ptr<FileAccess> file_access = *res;
    int buffer_length = (int)file_access->get_length();
    std::vector<uint8_t> file_buffer;
    file_buffer.resize(buffer_length);
    file_access->read_buffer(file_buffer.data(), buffer_length);

    uint8_t* data = stbi_load_from_memory(file_buffer.data(), buffer_length, &width, &height, &num_channels, 0);
    if (!data) {
        LOG_ERROR("failed to load image '{}'", path);
        return nullptr;
    }
    DEV_ASSERT(data);

    if (width % 4 != 0 || height % 4 != 0) {
        stbi_image_free(data);
        data = stbi_load(path.c_str(), &width, &height, &num_channels, 4);
        num_channels = 4;
    }

    std::vector<uint8_t> buffer;

    buffer.resize(width * height * num_channels);
    memcpy(buffer.data(), data, buffer.size());

    stbi_image_free(data);

    PixelFormat format = FORMAT_UNKNOWN;
    switch (num_channels) {
        case 1:
            format = FORMAT_R8_UINT;
            break;
        case 2:
            format = FORMAT_R8G8_UINT;
            break;
        case 3:
            format = FORMAT_R8G8B8_UINT;
            break;
        case 4:
            format = FORMAT_R8G8B8A8_UINT;
            break;
        default:
            CRASH_NOW();
            break;
    }

    return new Image(format, width, height, num_channels, buffer);
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
        // Timer timer;
        switch (task.type) {
            case LOAD_TASK_IMAGE: {
                auto handle = (ImageHandle*)task.userdata;
                handle->data = AssetManager::singleton().load_image_sync_internal(task.asset_path);
                handle->state = ASSET_STATE_READY;
                s_asset_manager_glob.loaded_images.push(handle);
            } break;
            default: {
                Scene* scene = new Scene;
                std::expected<void, std::string> res;

               auto loader = Loader<Scene>::create(task.asset_path);
                DEV_ASSERT(loader);

                bool ok = loader->load(scene);
                if (!ok) {
                    const std::string& error = loader->get_error();
                    if (task.on_error) {
                        task.on_error(error);
                    } else {
                        LOG_FATAL("{}", res.error());
                    }
                    return;
                }
                task.on_success(scene);
            } break;
        }
        // LOG_VERBOSE("[AssetManager] asset '{}' loaded in {}", task.asset_path, timer.get_duration_string());
    }
}

std::shared_ptr<File> AssetManager::find_file(const std::string& path) {
    auto found = s_asset_manager_glob.text_cache.find(path);
    if (found != s_asset_manager_glob.text_cache.end()) {
        return found->second;
    }

    return nullptr;
}

std::shared_ptr<File> AssetManager::load_file_sync(const std::string& path) {
    auto found = s_asset_manager_glob.text_cache.find(path);
    if (found != s_asset_manager_glob.text_cache.end()) {
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
    s_asset_manager_glob.text_cache[path] = text;
    return text;
}

}  // namespace my
