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

static void create_empty_scene(Scene* scene) {
    Entity::set_seed();

    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    scene->create_camera(frame_size.x, frame_size.y);

    auto root = scene->create_transform_entity("world");
    scene->m_root = root;
    if (0) {
        auto light = scene->create_omni_light_entity("omni light", vec3(1));
        auto transform = scene->get_component<TransformComponent>(light);
        DEV_ASSERT(transform);
        constexpr float rx = glm::radians(-80.0f);
        constexpr float ry = glm::radians(0.0f);
        constexpr float rz = glm::radians(0.0f);
        transform->rotate(vec3(rx, ry, rz));

        scene->attach_component(light, root);
    }
}

bool SceneManager::initialize() {
    // create an empty scene
    Scene* scene = new Scene;
    create_empty_scene(scene);
    m_scene = scene;

    const std::string& path = DVAR_GET_STRING(project);
    if (!path.empty()) {
        request_scene(path);
    }

    return true;
}

void SceneManager::finalize() {}

bool SceneManager::try_swap_scene() {
    auto queued_scene = m_loading_queue.pop_all();

    if (queued_scene.empty()) {
        return false;
    }

    while (!queued_scene.empty()) {
        auto task = queued_scene.front();
        queued_scene.pop();
        DEV_ASSERT(task.scene);
        if (m_scene && !task.replace) {
            m_scene->merge(*task.scene);
            m_scene->update(0.0f);
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

void SceneManager::update(float dt) {
    OPTICK_EVENT();

    try_swap_scene();

    if (m_last_revision < m_revision) {
        Timer timer;
        auto event = std::make_shared<SceneChangeEvent>(m_scene);
        LOG_WARN("offload p_scene properly");
        m_app->get_event_queue().dispatch_event(event);
        LOG("[SceneManager] Detected p_scene changed from revision {} to revision {}, took {}", m_last_revision, m_revision, timer.get_duration_string());
        m_last_revision = m_revision;
    }

    SceneManager::get_scene().update(dt);
}

void SceneManager::queue_loaded_scene(Scene* p_scene, bool p_replace) {
    m_loading_queue.push({ p_replace, p_scene });
}

void SceneManager::request_scene(std::string_view p_path) {
    fs::path path{ p_path };
    auto ext = path.extension().string();
    if (ext == ".lua" || ext == ".scene") {
        AssetManager::singleton().load_scene_async(std::string(p_path), [](void* p_scene, void*) {
            DEV_ASSERT(p_scene);
            Scene* new_scene = static_cast<Scene*>(p_scene);
            new_scene->update(0.0f);
            SceneManager::singleton().queue_loaded_scene(new_scene, true);
        });
    } else {
        AssetManager::singleton().load_scene_async(std::string(p_path), [](void* p_scene, void*) {
            DEV_ASSERT(p_scene);
            Scene* new_scene = static_cast<Scene*>(p_scene);
            new_scene->update(0.0f);
            SceneManager::singleton().queue_loaded_scene(new_scene, false);
        });
    }
}

Scene& SceneManager::get_scene() {
    assert(singleton().m_scene);
    return *singleton().m_scene;
}

}  // namespace my
