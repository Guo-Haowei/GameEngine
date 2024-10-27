#include "asset_manager.h"

#include "assets/loader_assimp.h"
#include "assets/loader_stbi.h"
#include "assets/loader_tinygltf.h"
#include "core/framework/graphics_manager.h"
#include "core/io/file_access.h"
#include "core/os/threads.h"
#include "core/os/timer.h"
#include "lua_binding/lua_scene_binding.h"
#include "rendering/render_manager.h"
#include "rendering/rendering_dvars.h"
#include "scene/scene.h"

namespace my {

static struct {
    // @TODO: better wake up
    std::condition_variable wake_condition;
    std::mutex wake_mutex;
    // @TODO: better thread safe queue
    ConcurrentQueue<LoadTask> job_queue;
} s_asset_manager_glob;

// @TODO: refactor
class LoaderDeserialize : public Loader<Scene> {
public:
    LoaderDeserialize(const std::string& p_path) : Loader<Scene>{ p_path } {}

    static std::shared_ptr<Loader<Scene>> create(const std::string& p_path) {
        return std::make_shared<LoaderDeserialize>(p_path);
    }

    bool load(Scene* p_scene) {
        Archive archive;
        if (!archive.openRead(m_file_path)) {
            return false;
        }
        p_scene->m_replace = true;
        return p_scene->Serialize(archive);
    }
};

class LoaderLuaScript : public Loader<Scene> {
public:
    LoaderLuaScript(const std::string& p_path) : Loader<Scene>{ p_path } {}

    static std::shared_ptr<Loader<Scene>> create(const std::string& p_path) {
        return std::make_shared<LoaderLuaScript>(p_path);
    }

    bool load(Scene* p_scene) {
        ivec2 frame_size = DVAR_GET_IVEC2(resolution);
        p_scene->createCamera(frame_size.x, frame_size.y);
        auto root = p_scene->createTransformEntity("world");
        p_scene->m_replace = true;
        p_scene->m_root = root;
        return load_lua_scene(m_file_path, p_scene);
    }
};

bool AssetManager::initialize() {
    Loader<Scene>::register_loader(".obj", LoaderAssimp::create);
    Loader<Scene>::register_loader(".gltf", LoaderTinyGLTF::create);
    Loader<Scene>::register_loader(".scene", LoaderDeserialize::create);
    Loader<Scene>::register_loader(".lua", LoaderLuaScript::create);

    Loader<Image>::register_loader(".png", LoaderSTBI8::create);
    Loader<Image>::register_loader(".jpg", LoaderSTBI8::create);
    Loader<Image>::register_loader(".hdr", LoaderSTBI32::create);

    return true;
}

void AssetManager::finalize() {
    s_asset_manager_glob.wake_condition.notify_all();
}

void AssetManager::enqueueLoadTask(LoadTask& task) {
    s_asset_manager_glob.job_queue.push(std::move(task));
    s_asset_manager_glob.wake_condition.notify_one();
}

ImageHandle* AssetManager::findImage(const FilePath& p_path) {
    std::lock_guard guard(m_image_cache_lock);

    auto found = m_image_cache.find(p_path);
    if (found != m_image_cache.end()) {
        return found->second.get();
    }

    return nullptr;
}

ImageHandle* AssetManager::loadImageAsync(const FilePath& p_path, LoadSuccessFunc p_on_success) {
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
            GraphicsManager::singleton().requestTexture(handle);
        };
    }
    task.userdata = ret;
    task.asset_path = p_path;
    enqueueLoadTask(task);
    return ret;
}

ImageHandle* AssetManager::loadImageSync(const FilePath& p_path) {
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

    Image* image = new Image;
    if (!loader->load(image)) {
        delete image;
        return nullptr;
    }

    TextureDesc texture_desc{};
    SamplerDesc sampler_desc{};
    renderer::fill_texture_and_sampler_desc(image, texture_desc, sampler_desc);

    image->gpu_texture = GraphicsManager::singleton().createTexture(texture_desc, sampler_desc);
    handle->set(image);
    ImageHandle* ret = handle.get();
    m_image_cache[p_path] = std::move(handle);
    return ret;
}

void AssetManager::loadSceneAsync(const FilePath& p_path, LoadSuccessFunc p_on_success) {
    LoadTask task;
    task.type = LOAD_TASK_SCENE;
    task.asset_path = p_path;
    task.on_success = p_on_success;
    task.userdata = nullptr;
    enqueueLoadTask(task);
}

template<typename T>
static void load_asset(LoadTask& p_task, T* p_asset) {
    auto loader = Loader<T>::create(p_task.asset_path);
    const std::string& asset_path = p_task.asset_path.string();
    if (!loader) {
        LOG_ERROR("[AssetManager] not loader found for '{}'", asset_path);
        return;
    }

    Timer timer;
    if (loader->load(p_asset)) {
        p_task.on_success(p_asset, p_task.userdata);
        LOG_VERBOSE("[AssetManager] asset '{}' loaded in {}", asset_path, timer.getDurationString());
    } else {
        LOG_ERROR("[AssetManager] failed to load '{}', details: {}", asset_path, loader->get_error());
    }
}

void AssetManager::workerMain() {
    for (;;) {
        if (thread::ShutdownRequested()) {
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
                load_asset<Image>(task, new Image);
            } break;
            case LOAD_TASK_SCENE: {
                LOG_VERBOSE("[AssetManager] start loading scene {}", task.asset_path.string());
                load_asset<Scene>(task, new Scene);
            } break;
            default:
                CRASH_NOW();
                break;
        }
    }
}

std::shared_ptr<File> AssetManager::findFile(const FilePath& p_path) {
    auto found = m_text_cache.find(p_path);
    if (found != m_text_cache.end()) {
        return found->second;
    }

    return nullptr;
}

std::shared_ptr<File> AssetManager::loadFileSync(const FilePath& p_path) {
    auto found = m_text_cache.find(p_path);
    if (found != m_text_cache.end()) {
        return found->second;
    }

    auto res = FileAccess::open(p_path, FileAccess::READ);
    if (!res) {
        LOG_ERROR("[FileAccess] Error: failed to open file '{}', reason: {}", p_path.string(), res.error().getMessage());
        return nullptr;
    }

    std::shared_ptr<FileAccess> file_access = *res;

    const size_t size = file_access->getLength();

    std::vector<char> buffer;
    buffer.resize(size);
    file_access->readBuffer(buffer.data(), size);
    auto text = std::make_shared<File>();
    text->buffer = std::move(buffer);
    m_text_cache[p_path] = text;
    return text;
}

}  // namespace my
