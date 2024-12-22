#include "asset_manager.h"

#include <filesystem>

#include "engine/assets/asset_loader.h"
#include "engine/assets/gltf_loader.h"
#include "engine/core/framework/application.h"
#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/io/file_access.h"
#include "engine/core/os/threads.h"
#include "engine/core/os/timer.h"
#include "engine/core/string/string_builder.h"
#include "engine/scene/scene.h"

// plugins
#include "plugins/loader_assimp/assimp_asset_loader.h"

namespace my {

namespace fs = std::filesystem;

struct LoadTask {
    FilePath assetPath;
    OnAssetLoadSuccessFunc onSuccess;
    void* userdata;

    // new stuff
    AssetRegistryHandle* handle;
};

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
    IAssetLoader::RegisterLoader(".yaml", TextSceneLoader::CreateLoader);

    if constexpr (1) {
        IAssetLoader::RegisterLoader(".gltf", GltfLoader::CreateLoader);
    } else {
#if USING(USING_ASSIMP)
        IAssetLoader::RegisterLoader(".gltf", AssimpAssetLoader::CreateLoader);
#endif
    }
#if USING(USING_ASSIMP)
    IAssetLoader::RegisterLoader(".obj", AssimpAssetLoader::CreateLoader);
#endif

    IAssetLoader::RegisterLoader(".lua", TextAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".ttf", BufferAssetLoader::CreateLoader);

    IAssetLoader::RegisterLoader(".h", BufferAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".hlsl", BufferAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".glsl", BufferAssetLoader::CreateLoader);

    IAssetLoader::RegisterLoader(".png", ImageAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".jpg", ImageAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".hdr", ImageAssetLoader::CreateLoaderF);

    return Result<void>();
}

auto AssetManager::LoadAssetSync(AssetRegistryHandle* p_handle) -> Result<IAsset*> {
#if 0
    if (thread::GetThreadId() == thread::THREAD_MAIN) {
        LOG_WARN("Loading asset '{}' on main thread, this can be an expensive operation", p_handle->meta.path);
    }
#endif

    auto loader = IAssetLoader::Create(p_handle->meta);
    if (!loader) {
        return HBN_ERROR(ErrorCode::ERR_CANT_OPEN, "No suitable loader found for asset '{}'", p_handle->meta.path);
    }

    auto res = loader->Load();
    if (!res) {
        return HBN_ERROR(res.error());
    }

    IAsset* asset = *res;
    {
        // @TODO: thread safe?
        std::lock_guard lock(m_assetLock);
        m_assets.emplace_back(std::unique_ptr<IAsset>(asset));
        asset = m_assets.back().get();
    }

    p_handle->asset = asset;
    p_handle->meta.type = asset->type;
    p_handle->state = AssetRegistryHandle::ASSET_STATE_READY;
    asset->meta = p_handle->meta;

    if (asset->type == AssetType::IMAGE) {
        ImageAsset* image = dynamic_cast<ImageAsset*>(asset);

        // @TODO: based on render, create asset on work threads
        GraphicsManager::GetSingleton().RequestTexture(image);
    }

    return asset;
}

void AssetManager::LoadAssetAsync(AssetRegistryHandle* p_handle, OnAssetLoadSuccessFunc p_on_success, void* p_userdata) {
    LoadTask task;
    task.handle = p_handle;
    task.onSuccess = p_on_success;
    task.userdata = p_userdata;
    EnqueueLoadTask(task);
}

void AssetManager::FinalizeImpl() {
    RequestShutdown();

    m_assets.clear();
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
        auto res = AssetManager::GetSingleton().LoadAssetSync(task.handle);

        if (res) {
            IAsset* asset = *res;
            if (task.onSuccess) {
                task.onSuccess(asset, task.userdata);
            }
            LOG_VERBOSE("[AssetManager] asset '{}' loaded in {}", task.handle->meta.path, timer.GetDurationString());
        } else {
            StringStreamBuilder builder;
            builder << res.error();
            LOG_ERROR("{}", builder.ToString());
        }

        s_assetManagerGlob.runningWorkers.fetch_sub(1);
    }
}

void AssetManager::Wait() {
    while (s_assetManagerGlob.runningWorkers.load() != 0) {
        // dummy for loop
    }
}

void AssetManager::RequestShutdown() {
    s_assetManagerGlob.wakeCondition.notify_all();
}

}  // namespace my
