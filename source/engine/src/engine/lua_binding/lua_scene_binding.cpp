#include "lua_scene_binding.h"

#include "rendering/render_manager.h"
#include "sol/sol.hpp"

namespace my {

using ecs::Entity;

static bool try_get_float(const sol::table& p_table, const std::string& p_key, float& p_out) {
    sol::optional<float> f = p_table[p_key];
    if (!f) {
        return false;
    }

    p_out = *f;
    return true;
}

static bool try_get_vec3(const sol::table& p_table, const std::string& p_key, vec3& p_out) {
    sol::optional<sol::table> vec_array = p_table[p_key];
    if (!vec_array) {
        return false;
    }

    sol::optional<float> x = (*vec_array)[1];
    sol::optional<float> y = (*vec_array)[2];
    sol::optional<float> z = (*vec_array)[3];
    if (!x || !y || !x) {
        return false;
    }

    p_out = vec3(*x, *y, *z);
    return true;
}

static Entity lua_scene_create_entity(Scene* p_scene, const sol::table& p_components) {
    Entity id;
    Entity material_id;

    static int s_counter = 0;
    std::string name = std::format("Untitled-{}", ++s_counter);

    if (sol::optional<std::string> optional_name = p_components["name"]; optional_name) {
        name = *optional_name;
    }

    mat4 transform{ 1 };
    vec3 translate{ 0 };
    if (sol::optional<sol::table> table = p_components["transform"]; table) {
        mat4 translation_matrix{ 1 };
        mat4 rotation_matrix{ 1 };
        mat4 scale_matrix{ 1 };
        if (try_get_vec3(*table, "translate", translate)) {
            translation_matrix = glm::translate(translate);
        }
        if (vec3 rotate{ 0 }; try_get_vec3(*table, "rotate", rotate)) {
            mat4 rotation_x = glm::rotate(glm::radians(rotate.x), vec3(1, 0, 0));
            mat4 rotation_y = glm::rotate(glm::radians(rotate.y), vec3(0, 1, 0));
            mat4 rotation_z = glm::rotate(glm::radians(rotate.z), vec3(0, 0, 1));
            rotation_matrix = rotation_z * rotation_y * rotation_x;
        }
        if (vec3 scale{ 1 }; try_get_vec3(*table, "scale", scale)) {
            scale_matrix = glm::scale(scale);
        }

        transform = rotation_matrix * translation_matrix * scale_matrix;
        // transform = translation_matrix * rotation_matrix * scale_matrix;
    }

    if (sol::optional<std::string> type = p_components["type"]; type) {
        if (type == "POINT_LIGHT") {
            id = p_scene->CreatePointLightEntity(name, translate);
            LightComponent* light = p_scene->GetComponent<LightComponent>(id);
            light->SetCastShadow();
            light->SetDirty();
        } else if (type == "INFINITE_LIGHT") {
            id = p_scene->CreateInfiniteLightEntity(name);
            LightComponent* light = p_scene->GetComponent<LightComponent>(id);
            light->SetCastShadow();
            light->SetDirty();
        } else {
            CRASH_NOW();
        }
    }

    auto create_material = [&]() {
        return p_scene->CreateMaterialEntity(name + "-material");
    };

    if (sol::optional<sol::table> table = p_components["material"]; table) {
        material_id = create_material();
        MaterialComponent* material = p_scene->GetComponent<MaterialComponent>(material_id);
        vec3 color{ 1 };
        bool ok = true;
        ok = ok && try_get_float(*table, "roughness", material->roughness);
        ok = ok && try_get_float(*table, "metallic", material->metallic);
        DEV_ASSERT(ok);
        if (try_get_vec3(*table, "base_color", color)) {
            material->base_color = vec4(color, 1.0f);
        }
    }

    if (sol::optional<sol::table> table = p_components["cube"]; table) {
        vec3 size{ 0.5f };
        try_get_vec3(*table, "size", size);
        if (!material_id.IsValid()) {
            material_id = create_material();
        }

        id = p_scene->CreateCubeEntity(name, material_id, size, transform);
    }
    if (sol::optional<sol::table> table = p_components["plane"]; table) {
        vec3 size{ 0.5f };
        try_get_vec3(*table, "size", size);
        if (!material_id.IsValid()) {
            material_id = create_material();
        }

        id = p_scene->CreatePlaneEntity(name, material_id, size, transform);
    }
    if (sol::optional<sol::table> table = p_components["sphere"]; table) {
        float radius{ 0.5f };
        try_get_float(*table, "radius", radius);
        if (!material_id.IsValid()) {
            material_id = create_material();
        }

        id = p_scene->CreateSphereEntity(name, material_id, radius, transform);
    }

    if (TransformComponent* transform_component = p_scene->GetComponent<TransformComponent>(id); !transform_component) {
        DEV_ASSERT(!id.IsValid());
        id = p_scene->CreateNameEntity(name);

        transform_component = &p_scene->Create<TransformComponent>(id);
        transform_component->SetLocalTransform(transform);
    }

    if (sol::optional<sol::table> table = p_components["rigid_body"]; table) {
        if (sol::optional<std::string> shape = (*table)["shape"]; shape) {
            auto sss = *shape;
            unused(sss);
            float mass = 0.0f;
            if (try_get_float(*table, "mass", mass)) {
                if (*shape == "SHAPE_CUBE") {
                    auto& rigid_body = p_scene->Create<RigidBodyComponent>(id);
                    rigid_body.shape = RigidBodyComponent::SHAPE_CUBE;
                    rigid_body.mass = mass;
                    try_get_vec3(*table, "size", rigid_body.param.box.half_size);
                } else if (*shape == "SHAPE_SPHERE") {
                    auto& rigid_body = p_scene->Create<RigidBodyComponent>(id);
                    rigid_body.shape = RigidBodyComponent::SHAPE_SPHERE;
                    rigid_body.mass = mass;
                    try_get_float(*table, "radius", rigid_body.param.sphere.radius);
                } else {
                    CRASH_NOW();
                }
            }
        }
    }

    DEV_ASSERT(id.IsValid());
    return id;
}

struct LuaScene {
    LuaScene(Scene* p_scene) : scene(p_scene) {}

    uint32_t get_root() const { return scene->m_root.GetId(); }

    uint32_t create_entity(const sol::table& p_components) {
        uint32_t id = create_entity_detached(p_components);
        scene->AttachComponent(Entity{ id }, scene->m_root);
        return id;
    }

    uint32_t create_entity_detached(const sol::table& p_components) {
        Entity id = lua_scene_create_entity(scene, p_components);
        return id.GetId();
    }

    void attach(uint32_t p_child, uint32_t p_parent) {
        scene->AttachComponent(Entity{ p_child }, Entity{ p_parent });
    }

    void attach_root(uint32_t p_child) {
        scene->AttachComponent(Entity{ p_child }, Entity{ scene->m_root });
    }

    Scene* scene;
};

static void open_renderer_lib(sol::state& p_lua) {
    sol::table lib = p_lua.create_table();

    p_lua.set("Renderer", lib);
    lib.set_function("set_env_map", [](const std::string& p_env_path) {
        renderer::request_env_map(p_env_path);
    });
}

bool load_lua_scene(const std::string& p_path, Scene* p_scene) {
    sol::state lua;
    lua.open_libraries();

    // install libs
    open_renderer_lib(lua);

    sol::table lib = lua.create_table();

    lua.new_usertype<LuaScene>("LuaScene",
                               "get_root", &LuaScene::get_root,
                               "create_entity", &LuaScene::create_entity,
                               "create_entity_detached", &LuaScene::create_entity_detached,
                               "attach", &LuaScene::attach,
                               "attach_root", &LuaScene::attach_root);

    lua.set("Scene", lib);
    lib.set_function("get", [&]() { return LuaScene{ p_scene }; });
    lib.set_function("create_entity", lua_scene_create_entity);

    // add additional package path
    std::string package_path = lua["package"]["path"];

    // add search directory
    std::filesystem::path new_search_dir{ __FILE__ };
    new_search_dir = new_search_dir.remove_filename();
    new_search_dir.append("?.lua;");
    lua["package"]["path"] = new_search_dir.string() + package_path;

    lua.script_file(p_path);

    return true;
}

}  // namespace my
