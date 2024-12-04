#include "engine/core/framework/scene_manager.h"
#include "engine/core/math/color.h"
#include "engine/core/math/geometry.h"
#include "engine/renderer/graphics_dvars.h"

namespace my {

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

    // colors
    const auto red = Color::Hex(0xF25346);
    const auto white = Color::Hex(0XD8D0D1);
    const auto dark_brown = Color::Hex(0x23190F);
    const auto brown = Color::Hex(0x59332E);
    // auto pink = Color::Hex(0xF5986E);
    // auto blue = Color::Hex(0X68C3C0);

    ecs::Entity::SetSeed();

    Scene* scene = new Scene;

    Vector2i frame_size = DVAR_GET_IVEC2(resolution);
    scene->CreateCamera(frame_size.x, frame_size.y);

    auto root = scene->CreateTransformEntity("world");
    scene->m_root = root;

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

    ecs::Entity material_red = scene->CreateMaterialEntity("material_red");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_red);
        material->baseColor = red.ToVector4f();
    }
    ecs::Entity material_white = scene->CreateMaterialEntity("material_white");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_white);
        material->baseColor = white.ToVector4f();
    }
    ecs::Entity material_dark_brown = scene->CreateMaterialEntity("material_dark_brown");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_dark_brown);
        material->baseColor = dark_brown.ToVector4f();
    }
    ecs::Entity material_brown = scene->CreateMaterialEntity("material_brown");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_brown);
        material->baseColor = brown.ToVector4f();
    }

    // create plane
    auto plane = scene->CreateTransformEntity("plane");
    {
        scene->AttachChild(plane, root);
        auto& script = scene->Create<ScriptComponent>(plane);
        script.SetScript("@res://scripts/plane.lua");
    }
    {
        auto cockpit = scene->CreateMeshEntity("cockpit",
                                               MakeCubeMesh(cockpit_points),
                                               material_red);
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
    // @TODO: CreateConeEntity
    auto propeller = scene->CreateCylinderEntity("propeller",
                                                 material_brown,
                                                 0.5f,
                                                 0.5f,
                                                 glm::translate(Vector3f(6.0f, 0.0f, 0.0f)));
    {
        scene->AttachChild(propeller, plane);
        auto& script = scene->Create<ScriptComponent>(propeller);
        script.SetScript("@res://scripts/propeller.lua");
    }
    {
        auto blade1 = scene->CreateCubeEntity("blade1",
                                              material_dark_brown,
                                              Vector3f(0.2f, 4.0f, 0.2f),
                                              glm::translate(Vector3f(0.8f, 0.0f, 0.0f)));
        scene->AttachChild(blade1, propeller);
    }
    {
        auto blade2 = scene->CreateCubeEntity("blade2",
                                              material_dark_brown,
                                              Vector3f(0.2f, 0.2f, 4.0f),
                                              glm::translate(Vector3f(0.8f, 0.0f, 0.0f)));
        scene->AttachChild(blade2, propeller);
    }

    return scene;
}

}  // namespace my
