#include "scene_manager.h"

#include <imgui/imgui.h>

#include "assets/asset_loader.h"
#include "core/framework/application.h"
#include "core/framework/common_dvars.h"
#include "core/os/timer.h"
#include "rendering/rendering_dvars.h"
#include "servers/display_server.h"
// @TODO: fix

#include "rendering/r_cbuffers.h"

// @TODO: refactor
namespace my {

[[nodiscard]] std::vector<vec4> getFrustumCornersWorldSpace(const mat4& projview) {
    const auto inv = glm::inverse(projview);

    std::vector<vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x) {
        for (unsigned int y = 0; y < 2; ++y) {
            for (unsigned int z = 0; z < 2; ++z) {
                const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}

[[nodiscard]] static mat4 getLightSpaceMatrix(vec3 lightDir, float nearPlane, float farPlane, Camera& camera) {
    const mat4 proj = glm::perspective(camera.get_fovy().to_rad(), camera.get_width() / camera.get_height(), nearPlane, farPlane);
    const auto corners = getFrustumCornersWorldSpace(proj * camera.get_view_matrix());

    glm::vec3 center = glm::vec3(0, 0, 0);
    for (const auto& v : corners) {
        center += glm::vec3(v);
    }
    center /= corners.size();

    const auto lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : corners) {
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }

    // Tune this parameter according to the scene
    constexpr float zMult = 10.0f;
    if (minZ < 0) {
        minZ *= zMult;
    } else {
        minZ /= zMult;
    }
    if (maxZ < 0) {
        maxZ /= zMult;
    } else {
        maxZ *= zMult;
    }

    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    return lightProjection * lightView;
}

[[nodiscard]] std::vector<glm::mat4> getLightSpaceMatrices(vec3 light_dir, Camera& camera, const std::vector<float>& shadowCascadeLevels) {
    DEV_ASSERT(shadowCascadeLevels.size() == SC_NUM_CASCADES + 1);
    std::vector<glm::mat4> ret;
    for (size_t i = 1; i < shadowCascadeLevels.size(); ++i) {
        ret.push_back(getLightSpaceMatrix(light_dir, shadowCascadeLevels[i - 1], shadowCascadeLevels[i], camera));
    }
    return ret;
}
}  // namespace my

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

    scene->create_camera(800.0f, 600.0f);

    auto root = scene->create_transform_entity("world");
    scene->m_root = root;
    {
        auto light = scene->create_omnilight_entity("omni light", vec3(1), 20.f);
        auto transform = scene->get_component<TransformComponent>(light);
        DEV_ASSERT(transform);
        mat4 r = glm::rotate(glm::radians(30.0f), glm::vec3(0, 0, 1));
        transform->set_local_transform(r);

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

// @TODO: fix
// static mat4 R_HackLightSpaceMatrix(const vec3& lightDir) {
//    const Scene& scene = SceneManager::get_scene();
//    const vec3 center = scene.get_bound().center();
//    const vec3 extents = scene.get_bound().size();
//    const float size = 0.5f * glm::max(extents.x, glm::max(extents.y, extents.z));
//    const mat4 V = glm::lookAt(center + glm::normalize(lightDir) * size, center, vec3(0, 1, 0));
//    const mat4 P = glm::ortho(-size, size, -size, size, 0.0f, 2.0f * size);
//    return P * V;
//}

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

    if (frameW > 0 && frameH > 0) {
        camera.set_dimension((float)frameW, (float)frameH);
    }

    scene.update(dt);

    // THESE SHOULDN'T BE HERE
    auto& cache = g_perFrameCache.cache;
    const uint32_t light_count = glm::min<uint32_t>((uint32_t)scene.get_count<LightComponent>(), SC_LIGHT_MAX);
    DEV_ASSERT(light_count);

    cache.c_light_count = light_count;

    // auto camera = scene.m_camera;
    std::vector<mat4> light_matrices;
    const float z_near = camera.get_near();
    const float z_far = camera.get_far();
    const std::vector<float> shadowCascadeLevels{ z_near, z_far / 20.0f, z_far / 10.0f, z_far / 2.0f, z_far };

    for (uint32_t idx = 0; idx < light_count; ++idx) {
        const LightComponent& light_component = scene.get_component_array<LightComponent>()[idx];
        auto light_entity = scene.get_entity<LightComponent>(idx);
        const TransformComponent* light_transform = scene.get_component<TransformComponent>(light_entity);
        DEV_ASSERT(light_transform);

        Light& light = cache.c_lights[idx];
        light.cast_shadow = false;
        light.type = light_component.type;
        light.color = light_component.color * light_component.energy;
        switch (light_component.type) {
            case LightComponent::LIGHT_TYPE_OMNI: {
                vec3 light_dir = light_transform->get_local_matrix() * vec4(0, 1, 0, 0);
                light.cast_shadow = true;
                light.position = light_dir;

                light_matrices = getLightSpaceMatrices(-light_dir, camera, shadowCascadeLevels);
            } break;
            case LightComponent::LIGHT_TYPE_POINT: {
                light.atten_constant = light_component.atten.constant;
                light.atten_linear = light_component.atten.linear;
                light.atten_quadratic = light_component.atten.quadratic;
                light.position = light_transform->get_translation();
            } break;
            default:
                break;
        }
    }

    DEV_ASSERT(light_matrices.size() == SC_NUM_CASCADES);
    if (!light_matrices.empty()) {
        for (int idx = 0; idx < SC_NUM_CASCADES; ++idx) {
            cache.c_main_light_matrices[idx] = light_matrices[idx];
            cache.c_cascade_plane_distances[idx] = shadowCascadeLevels[idx + 1];
        }
    }

    cache.c_camera_position = camera.get_position();
    cache.c_view_matrix = camera.get_view_matrix();
    cache.c_projection_matrix = camera.get_projection_matrix();
    cache.c_projection_view_matrix = camera.get_projection_view_matrix();

    cache.c_enable_vxgi = DVAR_GET_BOOL(r_enable_vxgi);
    cache.c_debug_texture_id = DVAR_GET_INT(r_debug_texture);
    cache.c_no_texture = DVAR_GET_BOOL(r_no_texture);
    cache.c_debug_csm = DVAR_GET_BOOL(r_debug_csm);

    cache.c_screen_width = frameW;
    cache.c_screen_height = frameH;

    // SSAO
    cache.c_ssao_kernel_size = DVAR_GET_INT(r_ssaoKernelSize);
    cache.c_ssao_kernel_radius = DVAR_GET_FLOAT(r_ssaoKernelRadius);
    cache.c_ssao_noise_size = DVAR_GET_INT(r_ssaoNoiseSize);
    cache.c_enable_ssao = DVAR_GET_BOOL(r_enable_ssao);

    // c_fxaa_image
    cache.c_enable_fxaa = DVAR_GET_BOOL(r_enable_fxaa);

    // @TODO: refactor the following
    const int voxel_texture_size = DVAR_GET_INT(r_voxel_size);
    DEV_ASSERT(math::is_power_of_two(voxel_texture_size));
    DEV_ASSERT(voxel_texture_size <= 256);

    vec3 world_center = scene.get_bound().center();
    vec3 aabb_size = scene.get_bound().size();
    float world_size = glm::max(aabb_size.x, glm::max(aabb_size.y, aabb_size.z));

    const float max_world_size = DVAR_GET_FLOAT(r_vxgi_max_world_size);
    if (world_size > max_world_size) {
        world_center = camera.get_position();
        world_size = max_world_size;
    }

    const float texel_size = 1.0f / static_cast<float>(voxel_texture_size);
    const float voxel_size = world_size * texel_size;

    cache.c_world_center = world_center;
    cache.c_world_size_half = 0.5f * world_size;
    cache.c_texel_size = texel_size;
    cache.c_voxel_size = voxel_size;
}

void SceneManager::request_scene(std::string_view path, ImporterName importer) {
    asset_loader::load_scene_async(importer, std::string(path), [](void* scene) {
        DEV_ASSERT(scene);
        Scene* new_scene = static_cast<Scene*>(scene);
        SceneManager::singleton().set_loading_scene(new_scene);
    });
}

Scene& SceneManager::get_scene() {
    assert(singleton().m_scene);
    return *singleton().m_scene;
}

}  // namespace my
