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
#include "engine/renderer/render_manager.h"
#include "engine/scene/scene.h"

// plugins
#include "plugins/loader_assimp/assimp_asset_loader.h"

namespace my {

namespace fs = std::filesystem;

struct LoadTask {
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

auto AssetManager::InitializeImpl() -> Result<void> {
    IAssetLoader::RegisterLoader(".scene", SceneLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".lua", LuaSceneLoader::CreateLoader);

#if USING(USING_ASSIMP)
    IAssetLoader::RegisterLoader(".obj", AssimpAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".gltf", AssimpAssetLoader::CreateLoader);
#else
    IAssetLoader::RegisterLoader(".gltf", GltfAssetLoader::CreateLoader);
#endif

    IAssetLoader::RegisterLoader(".ttf", BufferAssetLoader::CreateLoader);

    IAssetLoader::RegisterLoader(".png", ImageAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".jpg", ImageAssetLoader::CreateLoader);
    // IAssetLoader::RegisterLoader(".hdr", ImageAssetLoader::CreateLoader);

    return Result<void>();
}

auto AssetManager::LoadAssetSync(AssetRegistryHandle* p_handle) -> Result<IAsset*> {
    if (thread::GetThreadId() == thread::THREAD_MAIN) {
        LOG_WARN("Loading asset '{}' on main thread, this can be an expensive operation", p_handle->meta.path);
    }

    auto loader = IAssetLoader::Create(p_handle->meta);
    if (!loader) {
        LOG_ERROR("No suitable loader found for asset '{}'", p_handle->meta.path);
        return nullptr;
    }

    auto res = loader->Load();
    if (!res) {
        return HBN_ERROR(res.error());
    }

    IAsset* asset = *res;
    {
        // @TODO: thread safe?
        std::lock_guard lock(m_assetLock);
        m_assets.emplace_back(std::move(std::unique_ptr<IAsset>(asset)));
        asset = m_assets.back().get();
    }

    p_handle->asset = asset;
    p_handle->meta.type = asset->type;
    p_handle->state = AssetRegistryHandle::ASSET_STATE_READY;

    if (asset->type == AssetType::IMAGE) {
        Image* image = dynamic_cast<Image*>(asset);

        // @TODO: based on render, create asset on work threads
        GraphicsManager::GetSingleton().RequestTexture(image);
    }

    return asset;
}

void AssetManager::LoadAssetAsync(AssetRegistryHandle* p_handle, LoadSuccessFunc p_on_success, void* p_userdata) {
    LoadTask task;
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

        Timer timer;
        // @TODO: return result
        auto res = AssetManager::GetSingleton().LoadAssetSync(task.handle);

        if (res) {
            IAsset* asset = *res;
            if (task.onSuccess) {
                task.onSuccess(asset, task.userdata);
            }
            LOG_VERBOSE("[AssetManager] asset '{}' loaded in {}", task.handle->meta.path, timer.GetDurationString());
        } else {
            LOG_FATAL("error handling");
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
