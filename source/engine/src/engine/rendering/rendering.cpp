#include "rendering.h"

#include "rendering/r_cbuffers.h"
#include "rendering/rendering_dvars.h"

namespace my {

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

[[nodiscard]] static mat4 get_light_space_matrix(const mat4& p_light_matrix, float p_near_plane, float p_far_plane, const Camera& p_camera) {
    // frustum corners in camera space
    Degree half_fovy = p_camera.get_fovy() * 0.5f;
    Degree half_fovx = p_camera.get_fovy() * 0.5f * p_camera.get_aspect();
    float tangent_half_fovy = half_fovy.tan();
    float tangent_half_fovx = half_fovx.tan();

    float xn = tangent_half_fovx * p_near_plane;
    float yn = tangent_half_fovy * p_near_plane;
    float xf = tangent_half_fovx * p_far_plane;
    float yf = tangent_half_fovy * p_far_plane;

    // clang-foramt off
    enum { A = 0,
           B,
           C,
           D,
           E,
           F,
           G,
           H };
    // clang-foramt on

    std::array<vec4, 8> corners{
        // near face
        vec4(+xn, +yn, p_near_plane, 1.0),
        vec4(-xn, +yn, p_near_plane, 1.0),
        vec4(+xn, -yn, p_near_plane, 1.0),
        vec4(-xn, -yn, p_near_plane, 1.0),

        // far face
        vec4(+xf, +yf, p_far_plane, 1.0),
        vec4(-xf, +yf, p_far_plane, 1.0),
        vec4(+xf, -yf, p_far_plane, 1.0),
        vec4(-xf, -yf, p_far_plane, 1.0)
    };

    vec3 light_dir = glm::normalize(p_light_matrix * vec4(0, 0, 1, 0));
    // vec3 light_up = glm::normalize(p_light_matrix * vec4(0, 1, 0, 0));
    vec3 light_up = vec3(0, 1, 0);
    mat4 light_view = glm::lookAt(2.0f * light_dir, vec3(0), light_up);
    // mat4 light_view = glm::lookAt(100.0f * light_dir, vec3(0), light_up);
    mat4 inv_view = glm::inverse(p_camera.get_view_matrix());

    AABB aabb;
    // frustum in world space, then to light space
    for (vec4& point : corners) {
        point = light_view * inv_view * point;
        aabb.expand_point(point);
    }

    float min_x = aabb.get_min().x;
    float max_x = aabb.get_max().x;
    float min_y = aabb.get_min().y;
    float max_y = aabb.get_max().y;
    float min_z = aabb.get_min().z;
    float max_z = aabb.get_max().z;

    min_z -= 20.f;
    max_z += 20.f;

    // constexpr float z_mult = 10.0f;
    // if (min_z < 0) {
    //     min_z *= z_mult;
    // } else {
    //     min_z /= z_mult;
    // }
    // if (max_z < 0) {
    //     max_z /= z_mult;
    // } else {
    //     max_z *= z_mult;
    // }

    mat4 light_projection = glm::ortho(min_x, max_x, min_y, max_y, min_z, max_z);
    return light_projection * light_view;

    /*
      F(-x,+y)_______________________ E(+x,+y)
            /|                      /|
           / |                     / |
          /  |                    /  |
       -x/___|___________________/A(+x,+y)
         |   |                   |   |
         |   |                   |   |
         |   |                   |   |
         |   |___________________|___| <--- far
         |  /                    |  /
         | /                     | /
        D|/______________________| <--- near
     */

    // vec3 center = 0.5f * (obb[E] + obb[D]);

    // mat4 light_view = glm::lookAt(center + yf * light_dir, center, )

    // now we want the light to be perpendicular

#if 0
    mat4 proj = glm::perspective(p_camera.get_fovy().to_rad(), p_camera.get_aspect(), p_near_plane, p_far_plane);
    const auto inv = glm::inverse(proj * p_camera.get_view_matrix());

    std::array<glm::vec4, 8> frustum_corners = {
        vec4{ -1, -1, -1, 1 },
        vec4{ -1, -1, +1, 1 },
        vec4{ -1, +1, -1, 1 },
        vec4{ -1, +1, +1, 1 },
        vec4{ +1, -1, -1, 1 },
        vec4{ +1, -1, +1, 1 },
        vec4{ +1, +1, -1, 1 },
        vec4{ +1, +1, +1, 1 },
    };
    for (vec4& point : frustum_corners) {
        point = inv * point;
        point = point / point.w;
    }

    glm::vec3 center{ 0 };
    for (const auto& v : frustum_corners) {
        center += glm::vec3(v);
    }
    center /= frustum_corners.size();

    vec3 light_dir = p_light_matrix * vec4(0, 1, 0, 0);
    vec3 light_up = p_light_matrix * vec4(0, 0, -1, 0);
    light_up = vec3(0, -1, 0);
    mat4 light_view = glm::lookAt(center + light_dir, center, light_up);

    AABB aabb;
    for (const vec4& corner : frustum_corners) {
        aabb.expand_point(corner);
    }

    //constexpr float factor = 0.3f;
    //float size_x = max_x - min_x;
    //float size_y = max_y - min_y;
    //min_x -= factor * size_x;
    //max_x += factor * size_x;
    //min_y -= factor * size_y;
    //max_y += factor * size_y;


//#if 0
    const glm::mat4 light_projection_matrix = glm::ortho(min_x, max_x, min_y, max_y, min_z, max_z);

    const mat4 projection_matrix = glm::perspective(p_camera.get_fovy().to_rad(), p_camera.get_aspect(), p_near_plane, p_far_plane);
    const auto corners = get_frustum_corner_world_space(projection_matrix * p_camera.get_view_matrix());

    glm::vec3 center{ 0 };
    for (const auto& corner : corners) {
        center += glm::vec3(corner);
    }
    center /= corners.size();

    AABB aabb;
    for (const auto& v : corners) {
        const auto trf = light_view_matrix * v;
        aabb.expand_point(trf);
    }

    // Tune this parameter according to the scene



    return light_projection_matrix * light_view_matrix;
#endif
}

[[nodiscard]] std::vector<mat4> get_light_space_matrices(const mat4& p_light_matrix, const Camera& p_camera, const vec4& p_cascade_end) {
    std::vector<glm::mat4> ret;
    for (int i = 0; i < SC_NUM_CASCADES; ++i) {
        float z_near = p_camera.get_near();
        // z_near = i == 0 ? z_near : p_cascade_end[i - 1];
        ret.push_back(get_light_space_matrix(p_light_matrix, z_near, p_cascade_end[i], p_camera));
    }
    return ret;
}

void fill_constant_buffers(const Scene& scene) {
    Camera& camera = *scene.m_camera.get();

    // THESE SHOULDN'T BE HERE
    auto& cache = g_perFrameCache.cache;
    const uint32_t light_count = glm::min<uint32_t>((uint32_t)scene.get_count<LightComponent>(), SC_LIGHT_MAX);
    DEV_ASSERT(light_count);

    cache.c_light_count = light_count;

    // auto camera = scene.m_camera;
    vec4 cascade_end = DVAR_GET_VEC4(cascade_end);
    std::vector<mat4> light_matrices;
    if (camera.get_far() != cascade_end.w) {
        camera.set_far(cascade_end.w);
        camera.set_dirty();
    }
    for (int idx = 0; idx < SC_NUM_CASCADES; ++idx) {
        float left = idx == 0 ? camera.get_near() : cascade_end[idx - 1];
        DEV_ASSERT(left < cascade_end[idx]);
    }

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
                mat4 light_matrix = light_transform->get_local_matrix();
                vec3 light_dir = glm::normalize(light_matrix * vec4(0, 0, 1, 0));
                light.cast_shadow = true;
                light.position = light_dir;

                light_matrices = get_light_space_matrices(light_matrix, camera, cascade_end);
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
        }
    }

    cache.c_cascade_plane_distances = cascade_end;

    cache.c_camera_position = camera.get_position();
    cache.c_view_matrix = camera.get_view_matrix();
    cache.c_projection_matrix = camera.get_projection_matrix();
    cache.c_projection_view_matrix = camera.get_projection_view_matrix();

    cache.c_enable_vxgi = DVAR_GET_BOOL(r_enable_vxgi);
    cache.c_debug_texture_id = DVAR_GET_INT(r_debug_texture);
    cache.c_no_texture = DVAR_GET_BOOL(r_no_texture);
    cache.c_debug_csm = DVAR_GET_BOOL(r_debug_csm);

    cache.c_screen_width = (int)camera.get_width();
    cache.c_screen_height = (int)camera.get_height();

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

}  // namespace my