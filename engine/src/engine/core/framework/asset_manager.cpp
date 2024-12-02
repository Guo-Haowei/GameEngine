#include "asset_manager.h"

#include <filesystem>

#include "engine/assets/asset_loader.h"
#include "engine/assets/loader_tinygltf.h"
#include "engine/core/framework/application.h"
#include "engine/core/framework/asset_registry.h"
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

enum class LoadTaskType {
    UNKNOWN,
    BUFFER,
    SCENE,
};

struct LoadTask {
    LoadTaskType type;
    FilePath assetPath;
    LoadSuccessFunc onSuccess;
    void* userdata;

    // new stuff
    AssetRegistryHandle* handle;
};

static std::mutex m_assetLock;
static std::vector<std::unique_ptr<IAsset>> m_assets;

static struct {
    // @TODO: better wake up
    std::condition_variable wakeCondition;
    std::mutex wakeMutex;
    // @TODO: better thread safe queue
    ConcurrentQueue<LoadTask> jobQueue;
    std::atomic_int runningWorkers;
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

    // @TODO: refactor, reload stuff
    IAssetLoader::RegisterLoader(".ttf", BufferAssetLoader::CreateLoader);

    IAssetLoader::RegisterLoader(".png", ImageAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".jpg", ImageAssetLoader::CreateLoader);
    // IAssetLoader::RegisterLoader(".hdr", ImageAssetLoader::CreateLoader);

    return Result<void>();
}

void AssetManager::LoadAssetAsync(AssetRegistryHandle* p_handle, LoadSuccessFunc p_on_success, void* p_userdata) {
    LoadTask task;
    task.type = LoadTaskType::UNKNOWN;

    task.handle = p_handle;
    task.onSuccess = p_on_success;
    task.userdata = p_userdata;
    EnqueueLoadTask(task);
}

void AssetManager::FinalizeImpl() {
    s_assetManagerGlob.wakeCondition.notify_all();
}

void AssetManager::EnqueueLoadTask(LoadTask& p_task) {
    s_assetManagerGlob.jobQueue.push(std::move(p_task));
    s_assetManagerGlob.wakeCondition.notify_one();
}

#if 0
ImageHandle* AssetManager::FindImage(const FilePath& p_path) {
    std::lock_guard guard(m_imageCacheLock);

    auto found = m_imageCache.find(p_path);
    if (found != m_imageCache.end()) {
        return found->second.get();
    }

    return nullptr;
}

ImageHandle* AssetManager::LoadImageAsync(const FilePath& p_path, LoadSuccessFunc p_on_success) {
    CRASH_NOW();
    m_imageCacheLock.lock();

    auto found = m_imageCache.find(p_path);
    if (found != m_imageCache.end()) {
        auto ret = found->second.get();
        m_imageCacheLock.unlock();
        return ret;
    }

    auto handle = std::make_unique<OldAssetHandle<Image>>();
    handle->state = ASSET_STATE_LOADING;
    ImageHandle* ret = handle.get();
    m_imageCache[p_path] = std::move(handle);
    m_imageCacheLock.unlock();

    LoadTask task;
    task.type = LoadTaskType::IMAGE;
    if (p_on_success) {
        task.onSuccess = p_on_success;
    } else {
        task.onSuccess = [](void* p_asset, void* p_userdata) {
        };
    }
    task.userdata = ret;
    task.assetPath = p_path;
    EnqueueLoadTask(task);
    return ret;
}

#endif

void AssetManager::LoadSceneAsync(const FilePath& p_path, LoadSuccessFunc p_on_success) {
    LoadTask task;
    task.type = LoadTaskType::SCENE;
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
        LOG_ERROR("[AssetManager] no loader found for '{}'", asset_path);
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

        s_assetManagerGlob.runningWorkers.fetch_add(1);

        // LOG_VERBOSE("[AssetManager] start loading asset '{}'", task.asset_path);
        switch (task.type) {
            case LoadTaskType::SCENE: {
                LOG_VERBOSE("[AssetManager] start loading scene {}", task.assetPath.String());
                LoadAsset<Scene>(task, new Scene);
            } break;
            case LoadTaskType::UNKNOWN: {
                auto loader = IAssetLoader::Create(task.handle->meta);
                if (!loader) {
                    LOG_ERROR("No suitable loader found for asset '{}'", task.handle->meta.path);
                    break;
                }

                IAsset* asset = loader->Load();
                task.handle->asset = asset;
                task.handle->meta.type = asset->type;
                task.handle->state = AssetRegistryHandle::ASSET_STATE_READY;

                // @TODO: thread safe?
                {
                    std::lock_guard lock(m_assetLock);
                    m_assets.emplace_back(std::move(std::unique_ptr<IAsset>(asset)));
                    asset = m_assets.back().get();
                }

                if (task.handle->meta.type == AssetType::IMAGE) {
                    Image* image = dynamic_cast<Image*>(asset);

                    // @TODO: based on render, create asset on work threads
                    GraphicsManager::GetSingleton().RequestTexture(image);
                }
                if (task.onSuccess) {
                    task.onSuccess(asset, task.userdata);
                }
            } break;
            default:
                CRASH_NOW();
                break;
        }

        s_assetManagerGlob.runningWorkers.fetch_sub(1);
    }
}

void AssetManager::Wait() {
    while (s_assetManagerGlob.runningWorkers.load() != 0) {
        // dummy for loop
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
