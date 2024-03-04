#include "scene_manager.h"

#include <imgui/imgui.h>

#include "core/debugger/profiler.h"
#include "core/framework/application.h"
#include "core/framework/asset_manager.h"
#include "core/framework/common_dvars.h"
#include "core/os/timer.h"
#include "lua_binding/lua_scene_binding.h"
#include "rendering/rendering_dvars.h"

namespace my {

using ecs::Entity;

static bool deserialize_scene(Scene* scene, const std::string& path) {
    Archive archive;
    if (!archive.open_read(path)) {
        return false;
    }

    return scene->serialize(archive);
}

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

    // @TODO: fix
    const std::string& path = DVAR_GET_STRING(project);
    if (path.empty()) {
        create_empty_scene(scene);
    } else {
        // @TODO: clean this up

        bool ok = true;
        std::filesystem::path sys_path{ path };
        if (sys_path.extension() == ".lua") {
            create_empty_scene(scene);
            ok = load_lua_scene(path, scene);
        } else {
            ok = deserialize_scene(scene, path);
        }
        if (!ok) {
            LOG_FATAL("failed to deserialize scene '{}'", path);
        }
    }

    m_loading_scene.store(scene);
    if (try_swap_scene()) {
        m_scene->update(0.0f);
    }

    return true;
}

void SceneManager::finalize() {}

bool SceneManager::try_swap_scene() {
    Scene* new_scene = m_loading_scene.load();
    if (new_scene) {
        if (m_scene && !new_scene->m_replace) {
            m_scene->merge(*new_scene);
            delete new_scene;
        } else {
            m_scene = new_scene;
            // @TODO: old scene
        }
        m_loading_scene.store(nullptr);
        ++m_revision;
        return true;
    }
    return false;
}

void SceneManager::update(float dt) {
    OPTICK_EVENT();

    try_swap_scene();

    if (m_last_revision < m_revision) {
        Timer timer;
        auto event = std::make_shared<SceneChangeEvent>(m_scene);
        LOG_WARN("offload scene properly");
        m_app->get_event_queue().dispatch_event(event);
        LOG("[SceneManager] Detected scene changed from revision {} to revision {}, took {}", m_last_revision, m_revision, timer.get_duration_string());
        m_last_revision = m_revision;
    }

    SceneManager::get_scene().update(dt);
}

// @TODO: add import and load scene
void SceneManager::request_scene(std::string_view path) {
    AssetManager::singleton().load_scene_async(std::string(path), [](void* scene, void*) {
        DEV_ASSERT(scene);
        Scene* new_scene = static_cast<Scene*>(scene);
        new_scene->update(0.0f);
        SceneManager::singleton().set_loading_scene(new_scene);
    });
}

Scene& SceneManager::get_scene() {
    assert(singleton().m_scene);
    return *singleton().m_scene;
}

}  // namespace my
