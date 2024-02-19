#include "scene_manager.h"

#include <imgui/imgui.h>

#include "core/framework/application.h"
#include "core/framework/asset_manager.h"
#include "core/framework/common_dvars.h"
#include "core/os/timer.h"
#include "servers/display_server.h"
// @TODO: fix

#include "rendering/r_cbuffers.h"

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
    Entity::set_seed(Entity::INVALID_ID + 1);

    scene->create_camera(800, 600);

    auto root = scene->create_transform_entity("world");
    scene->m_root = root;
    {
        auto light = scene->create_omnilight_entity("omni light", vec3(1), 20.f);
        auto transform = scene->get_component<TransformComponent>(light);
        DEV_ASSERT(transform);
        transform->rotate(vec3(glm::radians(80.0f), 0.0f, 0.0f));

        scene->attach_component(light, root);
    }
}

static void create_physics_test(Scene* scene) {
    create_empty_scene(scene);

    {
        vec3 half(3.f, .02f, 3.f);
        ecs::Entity ground = scene->create_cube_entity("Ground", half);
        scene->attach_component(ground, scene->m_root);
        RigidBodyComponent& rigidBody = scene->create<RigidBodyComponent>(ground);

        rigidBody.shape = RigidBodyComponent::SHAPE_BOX;
        rigidBody.mass = 0.0f;
        rigidBody.param.box.half_size = half;
    }

    for (int t = 0; t < 16; ++t) {
        int x = t % 8;
        int y = t / 8;
        vec3 translate = vec3(x - 3.f, 5 - 1.f * y, 0.0f);
        vec3 half = vec3(0.25f);

        // random rotation
        auto random_angle = []() {
            return glm::radians<float>(float(rand() % 180));
        };
        float rx = random_angle();
        float ry = random_angle();
        float rz = random_angle();
        mat4 R = glm::rotate(rx, vec3(1, 0, 0)) *
                 glm::rotate(ry, vec3(0, 1, 0)) *
                 glm::rotate(rz, vec3(0, 0, 1));

        vec3 _s, _t;
        vec4 rotation;
        decompose(R, _s, rotation, _t);

        std::string name = "Cube" + std::to_string(t);
        auto material_id = scene->create_material_entity(name);
        MaterialComponent& material = *scene->get_component<MaterialComponent>(material_id);
        material.base_color = vec4((float)x / 8, (float)y / 8, 0.8f, 1.0f);

        auto cube_id = scene->create_cube_entity(name, material_id, half, glm::translate(translate) * R);
        scene->attach_component(cube_id, scene->m_root);

        RigidBodyComponent& rigid_body = scene->create<RigidBodyComponent>(cube_id);
        rigid_body.shape = RigidBodyComponent::SHAPE_BOX;
        rigid_body.mass = 1.0f;
        rigid_body.param.box.half_size = half;
    }
}

bool SceneManager::initialize() {
    // create an empty scene
    Scene* scene = new Scene;

    // @TODO: fix
    if (DVAR_GET_BOOL(test_physics)) {
        create_physics_test(scene);
    } else {
        const std::string& path = DVAR_GET_STRING(project);
        if (path.empty()) {
            create_empty_scene(scene);
        } else {
            if (!deserialize_scene(scene, path)) {
                LOG_FATAL("failed to deserialize scene '{}'", path);
            }
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
        if (m_scene) {
            m_scene->merge(*new_scene);
            delete new_scene;
        } else {
            m_scene = new_scene;
        }
        m_loading_scene.store(nullptr);
        ++m_revision;
        return true;
    }
    return false;
}

void SceneManager::update(float dt) {
    try_swap_scene();

    if (m_last_revision < m_revision) {
        // @TODO: profiler
        Timer timer;
        auto event = std::make_shared<SceneChangeEvent>(m_scene);
        m_app->get_event_queue().dispatch_event(event);
        LOG("[SceneManager] Detected scene changed from revision {} to revision {}, took {}", m_last_revision, m_revision, timer.get_duration_string());
        m_last_revision = m_revision;
    }

    // @TODO: refactor
    Scene& scene = SceneManager::get_scene();

    auto [frameW, frameH] = DisplayServer::singleton().get_frame_size();

    // @TODO: refactor this
    DEV_ASSERT(scene.m_camera);
    Camera& camera = *scene.m_camera.get();

    // @TODO: event
    if (frameW > 0 && frameH > 0) {
        camera.set_dimension(frameW, frameH);
    }

    scene.update(dt);
}

void SceneManager::request_scene(std::string_view path) {
    AssetManager::singleton().load_scene_async(std::string(path), [](void* scene) {
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
