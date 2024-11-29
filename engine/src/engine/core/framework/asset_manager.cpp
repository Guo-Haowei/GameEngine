#include "asset_manager.h"

#include <filesystem>

#include "engine/assets/loader_stbi.h"
#include "engine/assets/loader_tinygltf.h"
#include "engine/core/framework/application.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/io/file_access.h"
#include "engine/core/os/threads.h"
#include "engine/core/os/timer.h"
#include "engine/lua_binding/lua_scene_binding.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/render_manager.h"
#include "engine/scene/scene.h"

// plugins
#include "plugins/loader_assimp/loader_assimp.h"

namespace my {

namespace fs = std::filesystem;

static struct {
    // @TODO: better wake up
    std::condition_variable wakeCondition;
    std::mutex wakeMutex;
    // @TODO: better thread safe queue
    ConcurrentQueue<LoadTask> jobQueue;
} s_assetManagerGlob;

// @TODO: refactor
class LoaderDeserialize : public Loader<Scene> {
public:
    LoaderDeserialize(const std::string& p_path) : Loader<Scene>{ p_path } {}

    static std::shared_ptr<Loader<Scene>> Create(const std::string& p_path) {
        return std::make_shared<LoaderDeserialize>(p_path);
    }

    bool Load(Scene* p_scene) override {
        Archive archive;
        if (!archive.OpenRead(m_filePath)) {
            return false;
        }
        p_scene->m_replace = true;
        return p_scene->Serialize(archive);
    }
};

class LoaderLuaScript : public Loader<Scene> {
public:
    LoaderLuaScript(const std::string& p_path) : Loader<Scene>{ p_path } {}

    static std::shared_ptr<Loader<Scene>> Create(const std::string& p_path) {
        return std::make_shared<LoaderLuaScript>(p_path);
    }

    bool Load(Scene* p_scene) override {
        Vector2i frame_size = DVAR_GET_IVEC2(resolution);
        p_scene->CreateCamera(frame_size.x, frame_size.y);
        auto root = p_scene->CreateTransformEntity("world");
        p_scene->m_replace = true;
        p_scene->m_root = root;
        return LoadLuaScene(m_filePath, p_scene);
    }
};

auto AssetManager::InitializeImpl() -> Result<void> {
#if USING(USING_ASSIMP)
    Loader<Scene>::RegisterLoader(".obj", LoaderAssimp::Create);
    Loader<Scene>::RegisterLoader(".gltf", LoaderAssimp::Create);
#else
    Loader<Scene>::RegisterLoader(".gltf", LoaderTinyGLTF::Create);
#endif
    Loader<Scene>::RegisterLoader(".scene", LoaderDeserialize::Create);
    Loader<Scene>::RegisterLoader(".lua", LoaderLuaScript::Create);

    Loader<Image>::RegisterLoader(".png", LoaderSTBI8::Create);
    Loader<Image>::RegisterLoader(".jpg", LoaderSTBI8::Create);
    Loader<Image>::RegisterLoader(".hdr", LoaderSTBI32::Create);

    fs::path assets_root = fs::path{ m_app->GetResourceFolder() };
    std::vector<const char*> asset_folders = {
        "animations",
        "materials",
        "meshes",
    };

    for (auto folder : asset_folders) {
        fs::path root = assets_root / folder;
        if (!fs::exists(root)) {
            continue;
        }

        for (const auto& entry : fs::directory_iterator(root)) {
            const bool is_file = entry.is_regular_file();
            if (!is_file) {
                continue;
            }

            fs::path full_path = entry.path();
            std::string extension = full_path.extension().string();
            if (extension == ".meta") {
                LOG_VERBOSE("file '{}' found", full_path.string());
            }
        }
    }

    return Result<void>();
}

void AssetManager::FinalizeImpl() {
    s_assetManagerGlob.wakeCondition.notify_all();
}

void AssetManager::EnqueueLoadTask(LoadTask& p_task) {
    s_assetManagerGlob.jobQueue.push(std::move(p_task));
    s_assetManagerGlob.wakeCondition.notify_one();
}

ImageHandle* AssetManager::FindImage(const FilePath& p_path) {
    std::lock_guard guard(m_imageCacheLock);

    auto found = m_imageCache.find(p_path);
    if (found != m_imageCache.end()) {
        return found->second.get();
    }

    return nullptr;
}

ImageHandle* AssetManager::LoadImageAsync(const FilePath& p_path, LoadSuccessFunc p_on_success) {
    m_imageCacheLock.lock();

    auto found = m_imageCache.find(p_path);
    if (found != m_imageCache.end()) {
        auto ret = found->second.get();
        m_imageCacheLock.unlock();
        return ret;
    }

    auto handle = std::make_unique<AssetHandle<Image>>();
    handle->state = ASSET_STATE_LOADING;
    ImageHandle* ret = handle.get();
    m_imageCache[p_path] = std::move(handle);
    m_imageCacheLock.unlock();

    LoadTask task;
    task.type = LOAD_TASK_IMAGE;
    if (p_on_success) {
        task.onSuccess = p_on_success;
    } else {
        task.onSuccess = [](void* p_asset, void* p_userdata) {
            Image* image = reinterpret_cast<Image*>(p_asset);
            ImageHandle* handle = reinterpret_cast<ImageHandle*>(p_userdata);
            DEV_ASSERT(image);
            DEV_ASSERT(handle);

            handle->Set(image);
            GraphicsManager::GetSingleton().RequestTexture(handle);
        };
    }
    task.userdata = ret;
    task.assetPath = p_path;
    EnqueueLoadTask(task);
    return ret;
}

ImageHandle* AssetManager::LoadImageSync(const FilePath& p_path) {
    std::lock_guard guard(m_imageCacheLock);

    auto found = m_imageCache.find(p_path);
    if (found != m_imageCache.end()) {
        DEV_ASSERT(found->second->state.load() == ASSET_STATE_READY);
        return found->second.get();
    }

    // LOG_VERBOSE("image {} not found in cache, loading...", path);
    auto handle = std::make_unique<AssetHandle<Image>>();
    auto loader = Loader<Image>::Create(p_path);
    if (!loader) {
        LOG_ERROR("No loader found for image '{}'", p_path.String());
        return nullptr;
    }

    Image* image = new Image;
    if (!loader->Load(image)) {
        LOG_ERROR("Failed to load image image '{}', {}", p_path.String(), loader->GetError());
        delete image;
        return nullptr;
    }

    GpuTextureDesc texture_desc{};
    SamplerDesc sampler_desc{};
    renderer::fill_texture_and_sampler_desc(image, texture_desc, sampler_desc);

    image->gpu_texture = GraphicsManager::GetSingleton().CreateTexture(texture_desc, sampler_desc);
    handle->Set(image);
    ImageHandle* ret = handle.get();
    m_imageCache[p_path] = std::move(handle);
    return ret;
}

void AssetManager::LoadSceneAsync(const FilePath& p_path, LoadSuccessFunc p_on_success) {
    LoadTask task;
    task.type = LOAD_TASK_SCENE;
    task.assetPath = p_path;
    task.onSuccess = p_on_success;
    task.userdata = nullptr;
    EnqueueLoadTask(task);
}

template<typename T>
static void LoadAsset(LoadTask& p_task, T* p_asset) {
    auto loader = Loader<T>::Create(p_task.assetPath);
    const std::string& asset_path = p_task.assetPath.String();
    if (!loader) {
        LOG_ERROR("[AssetManager] not loader found for '{}'", asset_path);
        return;
    }

    Timer timer;
    if (loader->Load(p_asset)) {
        p_task.onSuccess(p_asset, p_task.userdata);
        LOG_VERBOSE("[AssetManager] asset '{}' loaded in {}", asset_path, timer.GetDurationString());
    } else {
        LOG_ERROR("[AssetManager] failed to load '{}', details: {}", asset_path, loader->GetError());
    }
}

void AssetManager::WorkerMain() {
    for (;;) {
        if (thread::ShutdownRequested()) {
            break;
        }

        LoadTask task;
        if (!s_assetManagerGlob.jobQueue.pop(task)) {
            std::unique_lock<std::mutex> lock(s_assetManagerGlob.wakeMutex);
            s_assetManagerGlob.wakeCondition.wait(lock);
            continue;
        }

        // LOG_VERBOSE("[AssetManager] start loading asset '{}'", task.asset_path);
        switch (task.type) {
            case LOAD_TASK_IMAGE: {
                LoadAsset<Image>(task, new Image);
            } break;
            case LOAD_TASK_SCENE: {
                LOG_VERBOSE("[AssetManager] start loading scene {}", task.assetPath.String());
                LoadAsset<Scene>(task, new Scene);
            } break;
            default:
                CRASH_NOW();
                break;
        }
    }
}

std::shared_ptr<File> AssetManager::FindFile(const FilePath& p_path) {
    auto found = m_textCache.find(p_path);
    if (found != m_textCache.end()) {
        return found->second;
    }

    return nullptr;
}

auto AssetManager::LoadFileSync(const FilePath& p_path) -> Result<std::shared_ptr<File>> {
    auto found = m_textCache.find(p_path);
    if (found != m_textCache.end()) {
        return found->second;
    }

    auto res = FileAccess::Open(p_path, FileAccess::READ);
    if (!res) {
        return HBN_ERROR(res.error());
    }

    std::shared_ptr<FileAccess> file_access = *res;

    const size_t size = file_access->GetLength();

    std::vector<char> buffer;
    buffer.resize(size);
    file_access->ReadBuffer(buffer.data(), size);
    auto text = std::make_shared<File>();
    text->buffer = std::move(buffer);
    m_textCache[p_path] = text;
    return text;
}

}  // namespace my
