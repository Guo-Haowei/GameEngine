#include "scene_manager.h"

#include <imgui/imgui.h>

#include "core/debugger/profiler.h"
#include "core/framework/application.h"
#include "core/framework/asset_manager.h"
#include "core/framework/common_dvars.h"
#include "core/os/timer.h"
#include "rendering/rendering_dvars.h"

namespace my {

using ecs::Entity;
namespace fs = std::filesystem;

static void CreateEmptyScene(Scene* p_scene) {
    Entity::SetSeed();

    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    p_scene->CreateCamera(frame_size.x, frame_size.y);

    auto root = p_scene->CreateTransformEntity("world");
    p_scene->m_root = root;
    if (0) {
        auto light = p_scene->CreateInfiniteLightEntity("infinite light", vec3(1));
        auto transform = p_scene->GetComponent<TransformComponent>(light);
        DEV_ASSERT(transform);
        constexpr float rx = glm::radians(-80.0f);
        constexpr float ry = glm::radians(0.0f);
        constexpr float rz = glm::radians(0.0f);
        transform->Rotate(vec3(rx, ry, rz));

        p_scene->AttachComponent(light, root);
    }
}

bool SceneManager::Initialize() {
    // create an empty scene
    Scene* scene = new Scene;
    CreateEmptyScene(scene);
    m_scene = scene;

    const std::string& path = DVAR_GET_STRING(project);
    if (!path.empty()) {
        RequestScene(path);
    }

    return true;
}

void SceneManager::Finalize() {}

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
            delete task.scene;
        } else {
            Scene* old_scene = m_scene;
            m_scene = task.scene;
            delete old_scene;
        }
        ++m_revision;
    }

    return true;
}

void SceneManager::Update(float p_elapsedTime) {
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

    SceneManager::GetScene().Update(p_elapsedTime);
}

void SceneManager::EnqueueSceneLoadingTask(Scene* p_scene, bool p_replace) {
    m_loadingQueue.push({ p_replace, p_scene });
}

void SceneManager::RequestScene(std::string_view p_path) {
    FilePath path{ p_path };

    std::string ext = path.Extension();
    if (ext == ".lua" || ext == ".scene") {
        AssetManager::GetSingleton().LoadSceneAsync(FilePath{ p_path }, [](void* p_scene, void*) {
            DEV_ASSERT(p_scene);
            Scene* new_scene = static_cast<Scene*>(p_scene);
            new_scene->Update(0.0f);
            SceneManager::GetSingleton().EnqueueSceneLoadingTask(new_scene, true);
        });
    } else {
        AssetManager::GetSingleton().LoadSceneAsync(FilePath{ p_path }, [](void* p_scene, void*) {
            DEV_ASSERT(p_scene);
            Scene* new_scene = static_cast<Scene*>(p_scene);
            new_scene->Update(0.0f);
            SceneManager::GetSingleton().EnqueueSceneLoadingTask(new_scene, false);
        });
    }
}

Scene& SceneManager::GetScene() {
    assert(GetSingleton().m_scene);
    return *GetSingleton().m_scene;
}

}  // namespace my
