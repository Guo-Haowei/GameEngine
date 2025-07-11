#include "asset_manager.h"

#include <filesystem>
#include <fstream>

#include "engine/assets/assets.h"
#include "engine/assets/asset_loader.h"
#include "engine/core/io/file_access.h"
#include "engine/core/os/threads.h"
#include "engine/core/os/timer.h"
#include "engine/core/string/string_builder.h"
#include "engine/renderer/graphics_manager.h"
#include "engine/runtime/application.h"
#include "engine/runtime/asset_registry.h"
#include "engine/scene/scene.h"

#if USING(PLATFORM_WINDOWS)
#define USE_TINYGLTF_LOADER IN_USE
#define USE_ASSIMP_LOADER   IN_USE
#elif USING(PLATFORM_APPLE)
#define USE_TINYGLTF_LOADER IN_USE
#define USE_ASSIMP_LOADER   NOT_IN_USE
#elif USING(PLATFORM_WASM)
#define USE_TINYGLTF_LOADER NOT_IN_USE
#define USE_ASSIMP_LOADER   NOT_IN_USE
#else
#error "Platform not supported"
#endif

#if USING(USE_TINYGLTF_LOADER)
#include "modules/tinygltf/tinygltf_loader.h"
#endif

// plugins
#include "plugins/loader_assimp/assimp_asset_loader.h"

namespace my {

namespace fs = std::filesystem;
using AssetCreateFunc = AssetRef (*)(void);

struct AssetManager::LoadTask {
    AssetEntry* handle;
    OnAssetLoadSuccessFunc on_success;
    void* userdata;
};

// @TODO: get rid of this?
static struct {
    // @TODO: better wake up
    std::condition_variable wakeCondition;
    std::mutex wakeMutex;
    // @TODO: better thread safe queue
    ConcurrentQueue<AssetManager::LoadTask> jobQueue;
    std::atomic_int runningWorkers;

    AssetCreateFunc createFuncs[AssetType::Count];
} s_assetManagerGlob;

auto AssetManager::InitializeImpl() -> Result<void> {
    m_assets_root = fs::path{ m_app->GetResourceFolder() };

    IAssetLoader::RegisterLoader(".scene", SceneLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".yaml", TextSceneLoader::CreateLoader);

#if USING(USE_TINYGLTF_LOADER)
    IAssetLoader::RegisterLoader(".gltf", TinyGLTFLoader::CreateLoader);
#elif USING(USE_ASSIMP_LOADER)
    IAssetLoader::RegisterLoader(".gltf", AssimpAssetLoader::CreateLoader);
#endif

#if USING(USING_ASSIMP)
    IAssetLoader::RegisterLoader(".obj", AssimpAssetLoader::CreateLoader);
#endif

    IAssetLoader::RegisterLoader(".sprite", TextAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".lua", TextAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".ttf", BufferAssetLoader::CreateLoader);

    IAssetLoader::RegisterLoader(".h", BufferAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".hlsl", BufferAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".glsl", BufferAssetLoader::CreateLoader);

    IAssetLoader::RegisterLoader(".png", ImageAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".jpg", ImageAssetLoader::CreateLoader);
    IAssetLoader::RegisterLoader(".hdr", ImageAssetLoader::CreateLoaderF);

    s_assetManagerGlob.createFuncs[AssetType::SpriteSheet] = []() {
        SpriteSheetAsset* sprite = new SpriteSheetAsset;

        sprite->frames.push_back({ Vector2f::Zero, Vector2f::One });

        return AssetRef(sprite);
    };

    return Result<void>();
}

void AssetManager::CreateAsset(const AssetType& p_type,
                               const fs::path& p_folder,
                               const char* p_name) {
    DEV_ASSERT(p_type == AssetType::SpriteSheet);
    // 1. Creates both meta and file
    fs::path new_file = p_folder;
    if (p_name) {
        new_file = new_file / p_name;
    } else {
        // @TODO: extension
        new_file = new_file / std::format("untitled{}.sprite", ++m_counter);
    }

    DEV_ASSERT(s_assetManagerGlob.createFuncs[p_type.GetData()]);
    AssetRef asset = s_assetManagerGlob.createFuncs[p_type.GetData()]();

    if (fs::exists(new_file)) {
        fs::remove(new_file);
    }
    std::ofstream file(new_file);
    asset->Serialize(file);

    std::string meta_file = new_file.string();
    meta_file.append(".meta");

    auto short_path = ResolvePath(new_file);
    auto _meta = AssetMetaData::CreateMeta(short_path);
    DEV_ASSERT(_meta);
    if (!_meta) {
        return;
    }
    auto meta = std::move(_meta.value());

    if (fs::exists(meta_file)) {
        fs::remove(meta_file);
    }

    auto res = meta.SaveMeta(meta_file);
    if (!res) {
        return;
    }

    // 2. Update AssetRegistry when done
    m_app->GetAssetRegistry()->StartAsyncLoad(std::move(meta), nullptr, nullptr);
}

std::string AssetManager::ResolvePath(const fs::path& p_path) {
    fs::path relative = fs::relative(p_path, m_assets_root);
    return std::format("@res://{}", relative.generic_string());
}

auto AssetManager::LoadAssetSync(const AssetEntry* p_entry) -> Result<AssetRef> {
    DEV_ASSERT(thread::GetThreadId() != thread::THREAD_MAIN);

    auto loader = IAssetLoader::Create(p_entry->metadata);
    if (!loader) {
        return HBN_ERROR(ErrorCode::ERR_CANT_OPEN, "No suitable loader found for asset '{}'", p_entry->metadata.path);
    }

    auto res = loader->Load();
    if (!res) {
        return HBN_ERROR(res.error());
    }

    AssetRef asset = *res;

    if (asset->type == AssetType::Image) {
        auto image = std::dynamic_pointer_cast<ImageAsset>(asset);

        // @TODO: based on render, create asset on work threads
        m_app->GetGraphicsManager()->RequestTexture(image.get());
    }

    LOG_VERBOSE("asset {} loaded", p_entry->metadata.path);
    return asset;
}

void AssetManager::LoadAssetAsync(AssetEntry* p_handle, OnAssetLoadSuccessFunc p_on_success, void* p_userdata) {
    LoadTask task;
    task.handle = p_handle;
    task.on_success = p_on_success;
    task.userdata = p_userdata;
    EnqueueLoadTask(task);
}

void AssetManager::FinalizeImpl() {
    RequestShutdown();
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
            AssetRef asset = *res;
            if (task.on_success) {
                task.on_success(asset, task.userdata);
            }
            LOG_VERBOSE("[AssetManager] asset '{}' loaded in {}", task.handle->metadata.path, timer.GetDurationString());

            task.handle->MarkLoaded(asset);
        } else {
            StringStreamBuilder builder;
            builder << res.error();
            LOG_ERROR("{}", builder.ToString());

            task.handle->MarkFailed();
        }

        s_assetManagerGlob.runningWorkers.fetch_sub(1);
    }
}

void AssetManager::RequestShutdown() {
    s_assetManagerGlob.wakeCondition.notify_all();
}

}  // namespace my
