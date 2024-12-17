#include "scene_manager.h"

#include <imgui/imgui.h>

#include "engine/core/debugger/profiler.h"
#include "engine/core/framework/application.h"
#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/common_dvars.h"
#include "engine/core/os/timer.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/scene/scene.h"

// @TODO: refactor
#include "engine/scene/scene_serialization.h"

namespace my {

using ecs::Entity;
namespace fs = std::filesystem;

auto SceneManager::InitializeImpl() -> Result<void> {
    m_scene = m_app->CreateInitialScene();
    BumpRevision();

    LOG_ERROR("TODO: remove this code");
    auto res = SaveSceneText("tmp.yaml", *m_scene);
    if (!res) {
        CRASH_NOW();
    }

    const std::string& path = DVAR_GET_STRING(project);
    if (!path.empty()) {
        RequestScene(path);
    }

    return Result<void>();
}

void SceneManager::FinalizeImpl() {}

bool SceneManager::TrySwapScene() {
    auto queued_scene = m_loadingQueue.pop_all();

    if (queued_scene.empty()) {
        return false;
    }

    while (!queued_scene.empty()) {
        auto task = queued_scene.front();
        queued_scene.pop();
        DEV_ASSERT(task.scene);
        if (m_scene && !task.replace) {
            m_scene->Merge(*task.scene);
            m_scene->Update(0.0f);
        } else {
            m_scene = task.scene;
        }
        ++m_revision;
    }

    return true;
}

void SceneManager::Update() {
    OPTICK_EVENT();

    TrySwapScene();

    if (m_lastRevision < m_revision) {
        Timer timer;
        auto event = std::make_shared<SceneChangeEvent>(m_scene);
        LOG_WARN("offload p_scene properly");
        m_app->GetEventQueue().DispatchEvent(event);
        LOG("[SceneManager] Detected p_scene changed from revision {} to revision {}, took {}", m_lastRevision, m_revision, timer.GetDurationString());
        m_lastRevision = m_revision;
    }
}

void SceneManager::EnqueueSceneLoadingTask(Scene* p_scene, bool p_replace) {
    m_loadingQueue.push({ p_replace, p_scene });
}

void SceneManager::RequestScene(std::string_view p_path) {
    FilePath path{ p_path };

    std::string ext = path.Extension();
    if (ext == ".lua" || ext == ".scene") {
        AssetRegistry::GetSingleton().RequestAssetAsync(path.String(), [](IAsset* p_scene, void*) {
            DEV_ASSERT(p_scene);
            Scene* new_scene = dynamic_cast<Scene*>(p_scene);
            new_scene->Update(0.0f);
            SceneManager::GetSingleton().EnqueueSceneLoadingTask(new_scene, true);
        });
    } else {
        AssetRegistry::GetSingleton().RequestAssetAsync(path.String(), [](IAsset* p_scene, void*) {
            DEV_ASSERT(p_scene);
            Scene* new_scene = dynamic_cast<Scene*>(p_scene);
            new_scene->Update(0.0f);
            SceneManager::GetSingleton().EnqueueSceneLoadingTask(new_scene, false);
        });
    }
}

}  // namespace my
