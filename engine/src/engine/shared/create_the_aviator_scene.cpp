#include "engine/core/base/random.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/math/color.h"
#include "engine/core/math/geometry.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/scene/scene.h"

namespace my {

static constexpr float OCEAN_RADIUS = 240.0f;
static constexpr int CLOUD_COUNT = 20;

// @TODO:
// * cascaded shadow map
// * alpha blending
// * collision
// * fog
// * dynamic buffer
// * 2D UI

Scene* CreateTheAviatorScene() {
    const std::array<Vector3f, 8> cockpit_points = {
        Vector3f(-4.f, +1.5f, +0.5f),  // A
        Vector3f(-4.f, +0.5f, +0.5f),  // B
        Vector3f(+4.f, -2.5f, +2.5f),  // C
        Vector3f(+4.f, +2.5f, +2.5f),  // D
        Vector3f(-4.f, +1.5f, -0.5f),  // E
        Vector3f(-4.f, +0.5f, -0.5f),  // F
        Vector3f(+4.f, -2.5f, -2.5f),  // G
        Vector3f(+4.f, +2.5f, -2.5f),  // H
    };

    const float plane_height = 20.0f;

    ecs::Entity::SetSeed();

    Scene* scene = new Scene;

    auto root = scene->CreateTransformEntity("root");
    scene->m_root = root;

    Vector2i frame_size = DVAR_GET_IVEC2(resolution);
    // editor camera
    {
        auto editor_camera = scene->CreatePerspectiveCameraEntity("editor_camera", frame_size.x, frame_size.y);
        auto camera = scene->GetComponent<PerspectiveCameraComponent>(editor_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0, plane_height, 80));
        camera->SetEditor();
        scene->AttachChild(editor_camera, root);
    }
    // main camera
    {
        auto main_camera = scene->CreatePerspectiveCameraEntity("main_camera", frame_size.x, frame_size.y);
        auto camera = scene->GetComponent<PerspectiveCameraComponent>(main_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0, plane_height, 100));
        camera->SetMain();
        scene->AttachChild(main_camera, root);
    }

    // create light
    auto light = scene->CreateInfiniteLightEntity("sun");
    {
        scene->AttachChild(light, root);
        TransformComponent* transform = scene->GetComponent<TransformComponent>(light);
        transform->RotateX(Degree(-41.f));
        transform->RotateY(Degree(-1.f));
        transform->RotateZ(Degree(-30.f));
        LightComponent* light_component = scene->GetComponent<LightComponent>(light);
        light_component->SetCastShadow();
    }

#pragma region SETUP_MATERIALS
    // colors
    const Color red = Color::Hex(0xCE190A);
    const Color white = Color::Hex(0XD8D0D1);
    const Color dark_brown = Color::Hex(0x23190F);
    const Color brown = Color::Hex(0x59332E);
    const Color blue = Color::Hex(0X10A8A3);
    // Color pink = Color::Hex(0xF5986E);

    constexpr float default_roughness = 0.8f;
    constexpr float default_metallic = 0.2f;
    ecs::Entity material_red = scene->CreateMaterialEntity("material_red");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_red);
        material->baseColor = red.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_white = scene->CreateMaterialEntity("material_white");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_white);
        material->baseColor = white.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_dark_brown = scene->CreateMaterialEntity("material_dark_brown");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_dark_brown);
        material->baseColor = dark_brown.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_brown = scene->CreateMaterialEntity("material_brown");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_brown);
        material->baseColor = brown.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_blue = scene->CreateMaterialEntity("material_blue");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_blue);
        material->baseColor = blue.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
#pragma endregion SETUP_MATERIALS

#pragma region SETUP_PLANE
    // create plane
    auto plane = scene->CreateTransformEntity("plane");
    {
        TransformComponent* transform = scene->GetComponent<TransformComponent>(plane);
        transform->Translate(Vector3f(0.0f, plane_height, 0.0f));

        scene->AttachChild(plane, root);
        auto& script = scene->Create<LuaScriptComponent>(plane);
        script.SetScript("@res://scripts/plane.lua");
    }
    {
        auto cockpit = scene->CreateMeshEntity("cockpit",
                                               material_red,
                                               MakeCubeMesh(cockpit_points));
        scene->AttachChild(cockpit, plane);
    }
    {
        auto engine = scene->CreateCubeEntity("engine",
                                              material_white,
                                              Vector3f(1.0f, 2.5f, 2.5f),
                                              glm::translate(Vector3f(5, 0, 0)));
        scene->AttachChild(engine, plane);
    }
    {
        auto side_wing = scene->CreateCubeEntity("side_wing",
                                                 material_red,
                                                 Vector3f(1.5f, 0.25f, 6.0f),
                                                 glm::translate(Vector3f(0.0f, 1.5f, 0.0f)));
        scene->AttachChild(side_wing, plane);
    }
    {
        auto tail = scene->CreateCubeEntity("tail",
                                            material_red,
                                            Vector3f(0.75f, 1.0f, 0.25f),
                                            glm::translate(Vector3f(-4, 2, 0)));
        scene->AttachChild(tail, plane);
    }
    {
        auto wind_shield = scene->CreateCubeEntity("wind_shield",
                                                   material_white,
                                                   Vector3f(0.15f, 0.75f, 1.0f),
                                                   glm::translate(Vector3f(0.5f, 2.7f, 0.0f)));
        scene->AttachChild(wind_shield, plane);
    }
    {
        auto wheel_protection_1 = scene->CreateCubeEntity("wheel_protection_1",
                                                          material_red,
                                                          Vector3f(1.5f, 0.75f, 0.5f),
                                                          glm::translate(Vector3f(2.5f, -2.0f, 2.5f)));
        scene->AttachChild(wheel_protection_1, plane);
    }
    {
        auto wheel_protection_2 = scene->CreateCubeEntity("wheel_protection_2",
                                                          material_red,
                                                          Vector3f(1.5f, 0.75f, 0.5f),
                                                          glm::translate(Vector3f(2.5f, -2.0f, -2.5f)));
        scene->AttachChild(wheel_protection_2, plane);
    }
    {
        auto tire_1 = scene->CreateCubeEntity("tire_1",
                                              material_dark_brown,
                                              Vector3f(1.2f, 1.2f, 0.2f),
                                              glm::translate(Vector3f(2.5f, -2.8f, 2.5f)));
        scene->AttachChild(tire_1, plane);
    }
    {
        auto tire_2 = scene->CreateCubeEntity("tire_2",
                                              material_dark_brown,
                                              Vector3f(1.2f, 1.2f, 0.2f),
                                              glm::translate(Vector3f(2.5f, -2.8f, -2.5f)));
        scene->AttachChild(tire_2, plane);
    }
    {
        auto wheel_axis = scene->CreateCubeEntity("wheel_axis",
                                                  material_brown,
                                                  Vector3f(0.5f, 0.5f, 2.95f),
                                                  glm::translate(Vector3f(2.5f, -2.8f, -0.0f)));
        scene->AttachChild(wheel_axis, plane);
    }

    {
        Matrix4x4f rotation = glm::rotate(Degree(160.f).GetRadians(), Vector3f(0, 0, 1));
        Matrix4x4f translation = glm::translate(Vector3f(-3.3f, -0.2f, 0.0f));
        auto suspension = scene->CreateCubeEntity("suspension",
                                                  material_red,
                                                  Vector3f(0.2f, 1.0f, 0.2f),
                                                  translation * rotation);
        scene->AttachChild(suspension, plane);
    }
    {
        auto tire_3 = scene->CreateCubeEntity("tire_3",
                                              material_brown,
                                              Vector3f(0.4f, 0.4f, 0.15f),
                                              glm::translate(Vector3f(-3.5f, -0.8f, 0.0f)));
        scene->AttachChild(tire_3, plane);
    }
    {
        auto tire_4 = scene->CreateCubeEntity("tire_4",
                                              material_dark_brown,
                                              Vector3f(0.6f, 0.6f, 0.1f),
                                              glm::translate(Vector3f(-3.5f, -0.8f, 0.0f)));
        scene->AttachChild(tire_4, plane);
    }

    auto propeller = scene->CreateTransformEntity("propeller");
    {
        TransformComponent* transform = scene->GetComponent<TransformComponent>(propeller);
        transform->Translate(Vector3f(6.0f, 0.0f, 0.0f));
        scene->AttachChild(propeller, plane);
        auto& script = scene->Create<LuaScriptComponent>(propeller);
        script.SetScript("@res://scripts/propeller.lua");
    }
    {
        auto pivot = scene->CreateMeshEntity("pivot",
                                             material_brown,
                                             MakeConeMesh(0.4f, 2.0f, 8));
        TransformComponent* transform = scene->GetComponent<TransformComponent>(pivot);
        transform->RotateZ(Degree(90.0f));
        scene->AttachChild(pivot, propeller);
    }
    {
        auto blade1 = scene->CreateCubeEntity("blade1",
                                              material_dark_brown,
                                              Vector3f(0.2f, 4.0f, 0.2f),
                                              glm::translate(Vector3f(0.4f, 0.0f, 0.0f)));
        scene->AttachChild(blade1, propeller);
    }
    {
        auto blade2 = scene->CreateCubeEntity("blade2",
                                              material_dark_brown,
                                              Vector3f(0.2f, 0.2f, 4.0f),
                                              glm::translate(Vector3f(0.4f, 0.0f, 0.0f)));
        scene->AttachChild(blade2, propeller);
    }

    auto world = scene->CreateTransformEntity("world");
    {
        scene->AttachChild(world, root);
        auto transform = scene->GetComponent<TransformComponent>(world);
        transform->Translate(Vector3f(0.0f, -OCEAN_RADIUS, 0.0f));
    }

    auto earth = scene->CreateTransformEntity("earth");
    {
        auto& script = scene->Create<LuaScriptComponent>(world);
        script.SetScript("@res://scripts/world.lua");
        scene->AttachChild(earth, world);
    }
    // ocean
    {
        auto ocean = scene->CreateMeshEntity("ocean", material_blue, MakeCylinderMesh(OCEAN_RADIUS, 320.0f, 60, 16));
        ObjectComponent* object = scene->GetComponent<ObjectComponent>(ocean);
        DEV_ASSERT(object);

        MeshComponent* mesh = scene->GetComponent<MeshComponent>(object->meshId);
        DEV_ASSERT(mesh);
        mesh->flags |= MeshComponent::DYNAMIC;

        auto transform = scene->GetComponent<TransformComponent>(ocean);
        DEV_ASSERT(transform);
        transform->RotateX(Degree(90.0f));
        transform->RotateZ(Degree(90.0f));
        scene->AttachChild(ocean, earth);
    }
#pragma endregion SETUP_PLANE

#pragma region SETUP_SKY
    constexpr float TWO_PI = 2.0f * glm::pi<float>();
    auto create_cloud = [&](int p_index, ecs::Entity p_parent) {
        int block = 3 + static_cast<int>(glm::floor(Random::Float() * 3));
        for (int block_index = 0; block_index < block; ++block_index) {
            std::string name = std::format("block_{}_{}", p_index, block_index);
            const float scale = 0.6f + Random::Float() * 0.4f;
            auto cloud = scene->CreateCubeEntity(name, material_white, Vector3f(5.0f * scale));
            auto transform = scene->GetComponent<TransformComponent>(cloud);
            Vector3f position(block_index * 6.0f, Random::Float() * 6.0f, Random::Float() * 6.0f);
            Vector3f rotation(0.0f, TWO_PI * Random::Float(), TWO_PI * Random::Float());
            transform->Rotate(rotation);
            transform->SetTranslation(position);

            scene->AttachChild(cloud, p_parent);
        }
    };

    constexpr float step_angle = TWO_PI / CLOUD_COUNT;
    for (int cloud_index = 0; cloud_index < CLOUD_COUNT; ++cloud_index) {
        const float angle = step_angle * cloud_index;
        std::string name = std::format("cloud_{}", cloud_index);
        auto cloud = scene->CreateTransformEntity(name);
        scene->AttachChild(cloud, earth);

        auto transform = scene->GetComponent<TransformComponent>(cloud);
        const float x = glm::sin(angle) * (OCEAN_RADIUS + 40.0f);
        const float y = glm::cos(angle) * (OCEAN_RADIUS + 40.0f);
        Matrix4x4f matrix = glm::translate(Vector3f(x, y, -50.0f)) * glm::rotate(angle, Vector3f(0, 0, -1));
        transform->SetLocalTransform(matrix);
        create_cloud(cloud_index, cloud);
    }
#pragma endregion SETUP_SKY

    {
        auto sky_light = scene->CreateHemisphereLightEntity("sky_light", "@res://images/ibl/sky.hdr");
        scene->AttachChild(sky_light, root);
    }

    return scene;
}

}  // namespace my
