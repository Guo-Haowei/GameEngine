#include "lua_scene_binding.h"

#include "engine/renderer/render_manager.h"

namespace my {

using ecs::Entity;

#if 0
static bool TryGetFloat(const sol::table& p_table, const std::string& p_key, float& p_out) {
    sol::optional<float> f = p_table[p_key];
    if (!f) {
        return false;
    }

    p_out = *f;
    return true;
}

static bool TryGetVec3(const sol::table& p_table, const std::string& p_key, Vector3f& p_out) {
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

    p_out = Vector3f(*x, *y, *z);
    return true;
}

static Entity LuaSceneCreateEntity(Scene* p_scene, const sol::table& p_components) {
    Entity id;
    Entity material_id;

    static int s_counter = 0;
    std::string name = std::format("Untitled-{}", ++s_counter);

    if (sol::optional<std::string> optional_name = p_components["name"]; optional_name) {
        name = *optional_name;
    }

    Matrix4x4f transform{ 1 };
    Vector3f translate{ 0 };
    if (sol::optional<sol::table> table = p_components["transform"]; table) {
        Matrix4x4f translation_matrix{ 1 };
        Matrix4x4f rotation_matrix{ 1 };
        Matrix4x4f scale_matrix{ 1 };
        if (TryGetVec3(*table, "translate", translate)) {
            translation_matrix = glm::translate(translate);
        }
        if (Vector3f rotate{ 0 }; TryGetVec3(*table, "rotate", rotate)) {
            Matrix4x4f rotation_x = glm::rotate(glm::radians(rotate.x), Vector3f(1, 0, 0));
            Matrix4x4f rotation_y = glm::rotate(glm::radians(rotate.y), Vector3f(0, 1, 0));
            Matrix4x4f rotation_z = glm::rotate(glm::radians(rotate.z), Vector3f(0, 0, 1));
            rotation_matrix = rotation_z * rotation_y * rotation_x;
        }
        if (Vector3f scale{ 1 }; TryGetVec3(*table, "scale", scale)) {
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
        Vector3f color{ 1 };
        bool ok = true;
        ok = ok && TryGetFloat(*table, "roughness", material->roughness);
        ok = ok && TryGetFloat(*table, "metallic", material->metallic);
        DEV_ASSERT(ok);
        if (TryGetVec3(*table, "base_color", color)) {
            material->baseColor = Vector4f(color, 1.0f);
        }
    }

    if (sol::optional<sol::table> table = p_components["cube"]; table) {
        Vector3f size{ 0.5f };
        TryGetVec3(*table, "size", size);
        if (!material_id.IsValid()) {
            material_id = create_material();
        }

        id = p_scene->CreateCubeEntity(name, material_id, size, transform);
    }
    if (sol::optional<sol::table> table = p_components["plane"]; table) {
        Vector3f size{ 0.5f };
        TryGetVec3(*table, "size", size);
        if (!material_id.IsValid()) {
            material_id = create_material();
        }

        id = p_scene->CreatePlaneEntity(name, material_id, size, transform);
    }
    if (sol::optional<sol::table> table = p_components["sphere"]; table) {
        float radius{ 0.5f };
        TryGetFloat(*table, "radius", radius);
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
            if (TryGetFloat(*table, "mass", mass)) {
                if (*shape == "SHAPE_CUBE") {
                    auto& rigid_body = p_scene->Create<RigidBodyComponent>(id);
                    rigid_body.shape = RigidBodyComponent::SHAPE_CUBE;
                    rigid_body.mass = mass;
                    TryGetVec3(*table, "size", rigid_body.param.box.half_size);
                } else if (*shape == "SHAPE_SPHERE") {
                    auto& rigid_body = p_scene->Create<RigidBodyComponent>(id);
                    rigid_body.shape = RigidBodyComponent::SHAPE_SPHERE;
                    rigid_body.mass = mass;
                    TryGetFloat(*table, "radius", rigid_body.param.sphere.radius);
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

    uint32_t GetRoot() const { return scene->m_root.GetId(); }

    uint32_t CreateEntity(const sol::table& p_components) {
        uint32_t id = CreateEntityDetached(p_components);
        scene->AttachComponent(Entity{ id }, scene->m_root);
        return id;
    }

    uint32_t CreateEntityDetached(const sol::table& p_components) {
        Entity id = LuaSceneCreateEntity(scene, p_components);
        return id.GetId();
    }

    void Attach(uint32_t p_child, uint32_t p_parent) {
        scene->AttachComponent(Entity{ p_child }, Entity{ p_parent });
    }

    void AttachRoot(uint32_t p_child) {
        scene->AttachComponent(Entity{ p_child }, Entity{ scene->m_root });
    }

    Scene* scene;
};
#endif

bool LoadLuaScene(const std::string& , Scene* ) {
    return false;
    //sol::state lua;
    //lua.open_libraries();

    //sol::table lib = lua.create_table();

    //lua.new_usertype<LuaScene>("LuaScene",
    //                           "get_root", &LuaScene::GetRoot,
    //                           "create_entity", &LuaScene::CreateEntity,
    //                           "create_entity_detached", &LuaScene::CreateEntityDetached,
    //                           "attach", &LuaScene::Attach,
    //                           "attach_root", &LuaScene::AttachRoot);

    //lua.set("Scene", lib);
    //lib.set_function("get", [&]() { return LuaScene{ p_scene }; });
    //lib.set_function("create_entity", LuaSceneCreateEntity);

    //// add additional package path
    //std::string package_path = lua["package"]["path"];

    //// add search directory
    //std::filesystem::path new_search_dir{ __FILE__ };
    //new_search_dir = new_search_dir.remove_filename();
    //new_search_dir.append("?.lua;");
    //lua["package"]["path"] = new_search_dir.string() + package_path;

    //lua.script_file(p_path);

    //return true;
}

}  // namespace my
