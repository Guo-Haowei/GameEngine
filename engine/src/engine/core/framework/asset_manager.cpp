#include "asset_manager.h"

#include <filesystem>

#include "engine/assets/loader_stbi.h"
#include "engine/assets/loader_tinygltf.h"
#include "engine/core/framework/application.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/io/file_access.h"
#include "engine/core/os/threads.h"
#include "engine/core/os/timer.h"
#include "engine/core/string/string_utils.h"
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
    IMAGE,
    BUFFER,
    SCENE,
};

struct LoadTask {
    LoadTaskType type;
    FilePath assetPath;
    LoadSuccessFunc onSuccess;
    void* userdata;

    // new stuff
    AssetMetaData meta;
};

static std::vector<std::unique_ptr<IAsset>> m_assets;

class IAssetLoader {
    using CreateLoaderFunc = std::unique_ptr<IAssetLoader> (*)(const AssetMetaData& p_meta);

public:
    IAssetLoader(const AssetMetaData& p_meta) : m_meta(p_meta) {}

    virtual ~IAssetLoader() = default;

    virtual IAsset* Load() = 0;

    static bool RegisterLoader(const std::string& p_extension, CreateLoaderFunc p_func) {
        DEV_ASSERT(!p_extension.empty());
        DEV_ASSERT(p_func);
        auto it = s_loaderCreator.find(p_extension);
        if (it != s_loaderCreator.end()) {
            LOG_WARN("Already exists a loader for p_extension '{}'", p_extension);
            it->second = p_func;
        } else {
            s_loaderCreator[p_extension] = p_func;
        }
        return true;
    }

    static std::unique_ptr<IAssetLoader> Create(const AssetMetaData& p_meta) {
        std::string_view extension = StringUtils::Extension(p_meta.path);
        auto it = s_loaderCreator.find(std::string(extension));
        if (it == s_loaderCreator.end()) {
            return nullptr;
        }
        return it->second(p_meta);
    }

    inline static std::map<std::string, CreateLoaderFunc> s_loaderCreator;

protected:
    const AssetMetaData& m_meta;
};

class BufferAssetLoader : public IAssetLoader {
public:
    using IAssetLoader::IAssetLoader;

    static std::unique_ptr<IAssetLoader> CreateLoader(const AssetMetaData& p_meta) {
        return std::make_unique<BufferAssetLoader>(p_meta);
    }

    virtual IAsset* Load() override {
        auto res = FileAccess::Open(m_meta.path, FileAccess::READ);
        if (!res) {
            LOG_FATAL("TODO: handle error");
            return nullptr;
        }

        std::shared_ptr<FileAccess> file_access = *res;

        const size_t size = file_access->GetLength();

        std::vector<char> buffer;
        buffer.resize(size);
        file_access->ReadBuffer(buffer.data(), size);
        auto file = new File;
        file->meta = m_meta;
        file->buffer = std::move(buffer);
        return file;
    }
};

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

    Loader<Image>::RegisterLoader(".png", LoaderSTBI8::Create);
    Loader<Image>::RegisterLoader(".jpg", LoaderSTBI8::Create);
    Loader<Image>::RegisterLoader(".hdr", LoaderSTBI32::Create);

    // @TODO: refactor, reload stuff
    IAssetLoader::RegisterLoader(".ttf", BufferAssetLoader::CreateLoader);

    return Result<void>();
}

void AssetManager::LoadAssetAsync(const AssetMetaData& p_meta, LoadSuccessFunc p_on_success, void* p_userdata) {
    LoadTask task;
    task.type = LoadTaskType::UNKNOWN;
    task.meta = p_meta;
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
    auto handle = std::make_unique<OldAssetHandle<Image>>();
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
            case LoadTaskType::IMAGE: {
                LoadAsset<Image>(task, new Image);
            } break;
            case LoadTaskType::SCENE: {
                LOG_VERBOSE("[AssetManager] start loading scene {}", task.assetPath.String());
                LoadAsset<Scene>(task, new Scene);
            } break;
            case LoadTaskType::UNKNOWN: {
                auto loader = IAssetLoader::Create(task.meta);
                DEV_ASSERT(loader);
                auto asset = loader->Load();
                m_assets.emplace_back(std::unique_ptr<IAsset>(asset));
                task.onSuccess(asset, task.userdata);
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
