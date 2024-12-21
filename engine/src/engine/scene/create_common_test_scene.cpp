#include "engine/core/base/random.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/scene/scene.h"

/*
@TODO
    * draw array vs draw elements
    * refactor mesh
*/

namespace my {

Scene* CreateBoxScene() {
    ecs::Entity::SetSeed();

    Scene* scene = new Scene;
    scene->m_physicsMode = PhysicsMode::SIMULATION;

    auto root = scene->CreateTransformEntity("root");
    scene->m_root = root;

    Vector2i frame_size = DVAR_GET_IVEC2(resolution);
    // editor camera
    {
        auto editor_camera = scene->CreatePerspectiveCameraEntity("editor_camera", frame_size.x, frame_size.y);
        auto camera = scene->GetComponent<PerspectiveCameraComponent>(editor_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0, 4, 15));
        camera->SetEditorCamera();
        scene->AttachChild(editor_camera, root);
    }
    // main camera
    {
        auto main_camera = scene->CreatePerspectiveCameraEntity("main_camera", frame_size.x, frame_size.y);
        auto camera = scene->GetComponent<PerspectiveCameraComponent>(main_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0, 4, 15));
        camera->SetPrimary();
        scene->AttachChild(main_camera, root);
    }
    // add a light
    if constexpr (0) {
        auto id = scene->CreatePointLightEntity("point_light", Vector3f(0, 0, 0));
        scene->AttachChild(id, root);
        LightComponent* light = scene->GetComponent<LightComponent>(id);
        light->SetCastShadow();
    }

    auto world = scene->CreateTransformEntity("world");
    scene->AttachChild(world, root);

    auto white = scene->CreateMaterialEntity("white");
    {
        MaterialComponent* mat = scene->GetComponent<MaterialComponent>(white);
        mat->metallic = 0.3f;
        mat->roughness = 0.7f;
    }
    auto red = scene->CreateMaterialEntity("red");
    {
        MaterialComponent* mat = scene->GetComponent<MaterialComponent>(red);
        mat->baseColor = Vector4f(1, 0, 0, 1);
        mat->metallic = 0.3f;
        mat->roughness = 0.7f;
    }
    auto green = scene->CreateMaterialEntity("green");
    {
        MaterialComponent* mat = scene->GetComponent<MaterialComponent>(green);
        mat->baseColor = Vector4f(0, 1, 0, 1);
        mat->metallic = 0.3f;
        mat->roughness = 0.7f;
    }
    {
        constexpr float s = 5.0f;
        struct {
            std::string name;
            Matrix4x4f transform;
            ecs::Entity material;
        } wall_info[] = {
            { "wall_up", glm::translate(Vector3f(0, s, 0)), white },
            { "wall_down", glm::translate(Vector3f(0, -s, 0)), white },
            { "wall_left", glm::rotate(glm::radians(+90.0f), Vector3f(0, 0, 1)) * glm::translate(Vector3f(0, s, 0)), red },
            { "wall_right", glm::rotate(glm::radians(+90.0f), Vector3f(0, 0, 1)) * glm::translate(Vector3f(0, -s, 0)), green },
            { "wall_back", glm::rotate(glm::radians(+90.0f), Vector3f(1, 0, 0)) * glm::translate(Vector3f(0, -s, 0)), white },
        };

        for (int i = 0; i < array_length(wall_info); ++i) {
            const auto& info = wall_info[i];
            auto wall = scene->CreateCubeEntity(info.name, info.material, Vector3f(s, 0.2f, s), info.transform);
            scene->AttachChild(wall, world);
        }
    }

    return scene;
}

Scene* CreatePhysicsTestScene() {
    ecs::Entity::SetSeed();

    Scene* scene = new Scene;
    scene->m_physicsMode = PhysicsMode::SIMULATION;

    auto root = scene->CreateTransformEntity("root");
    scene->m_root = root;

    Vector2i frame_size = DVAR_GET_IVEC2(resolution);
    // editor camera
    {
        auto editor_camera = scene->CreatePerspectiveCameraEntity("editor_camera", frame_size.x, frame_size.y);
        auto camera = scene->GetComponent<PerspectiveCameraComponent>(editor_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0, 5, 12));
        camera->SetEditorCamera();
        scene->AttachChild(editor_camera, root);
    }
    // main camera
    {
        auto main_camera = scene->CreatePerspectiveCameraEntity("main_camera", frame_size.x, frame_size.y);
        auto camera = scene->GetComponent<PerspectiveCameraComponent>(main_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0, 5, 12));
        camera->SetPrimary();
        scene->AttachChild(main_camera, root);
    }
    // add a light
    {
        auto id = scene->CreatePointLightEntity("point_light", Vector3f(0, 5, 3));
        scene->AttachChild(id, root);
        LightComponent* light = scene->GetComponent<LightComponent>(id);
        light->SetCastShadow();
    }

    auto world = scene->CreateTransformEntity("world");
    scene->AttachChild(world, root);

    ecs::Entity material_id = scene->CreateMaterialEntity("material");

    {
        Vector3f ground_scale(5.0f, 0.1f, 5.0f);
        auto ground = scene->CreateCubeEntity("ground_left", material_id, ground_scale);
        scene->AttachChild(ground, world);
        scene->Create<RigidBodyComponent>(ground)
            .InitCube(ground_scale)
            .InitGhost();

        TransformComponent* transform = scene->GetComponent<TransformComponent>(ground);
        transform->SetTranslation(Vector3f(-3.0f, 0.0f, 0.0f));
        transform->RotateZ(-Degree(45.0f));
    }
    {
        Vector3f ground_scale(5.0f, 0.1f, 5.0f);
        auto ground = scene->CreateCubeEntity("ground_right", material_id, ground_scale);
        scene->AttachChild(ground, world);
        scene->Create<RigidBodyComponent>(ground)
            .InitCube(ground_scale)
            .InitGhost();

        TransformComponent* transform = scene->GetComponent<TransformComponent>(ground);
        transform->SetTranslation(Vector3f(3.0f, 0.0f, 0.0f));
        transform->RotateZ(Degree(45.0f));
    }

    {
        constexpr float s = 3.5f;
        constexpr float h = 2.0f;
        Vector3f p0(-s, h, +s);  // top left
        Vector3f p1(+s, h, +s);  // top right
        Vector3f p2(+s, h, -s);  // bottom right
        Vector3f p3(-s, h, -s);  // bottom left

        auto cloth = scene->CreateClothEntity("cloth",
                                              material_id,
                                              p0, p1, p2, p3,
                                              Vector2i(30),
                                              CLOTH_FIX_ALL);

        scene->AttachChild(cloth, root);
    }

    for (int t = 1; t <= 21; ++t) {
        const int x = (t - 1) % 7;
        const int y = (t - 1) / 7;

        Vector3f translate(x - 3, 7 - y, 0);
        translate.x += 0.1f * (Random::Float() - 0.5f);
        translate.z += 0.1f * (Random::Float() - 0.5f);
        Vector3f scale(0.25f);

        ecs::Entity id;
        if (t % 2) {
            id = scene->CreateCubeEntity(std::format("Cube_{}", t), material_id, scale, glm::translate(translate));
            scene->Create<RigidBodyComponent>(id).InitCube(scale);
        } else {
            const float radius = 0.25f;
            id = scene->CreateSphereEntity(std::format("Sphere_{}", t), material_id, radius, glm::translate(translate));
            scene->Create<RigidBodyComponent>(id).InitSphere(radius);
        }
        scene->AttachChild(id, world);
    }

    return scene;
}

Scene* CreatePbrTestScene() {
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
        camera->SetPosition(Vector3f(0, 4, 10));
        camera->SetEditorCamera();
        scene->AttachChild(editor_camera, root);
    }
    // main camera
    {
        auto main_camera = scene->CreatePerspectiveCameraEntity("main_camera", frame_size.x, frame_size.y);
        auto camera = scene->GetComponent<PerspectiveCameraComponent>(main_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0, 4, 10));
        camera->SetPrimary();
        scene->AttachChild(main_camera, root);
    }

    auto world = scene->CreateTransformEntity("world");
    scene->AttachChild(world, root);

    const int num_row = 7;
    const int num_col = 7;
    const float spacing = 1.2f;

    for (int row = 0; row < num_row; ++row) {
        for (int col = 0; col < num_col; ++col) {
            const float x = (col - 0.5f * num_col) * spacing;
            const float y = (row - 0.5f * num_row) * spacing;

            auto name = std::format("sphere_{}_{}", row, col);
            auto material_id = scene->CreateMaterialEntity(std::format("{}_mat", name));
            MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_id);
            material->metallic = (float)row / (num_row - 1);
            material->roughness = (float)col / (num_col - 1);
            material->metallic = glm::clamp(material->metallic, 0.05f, 0.95f);
            material->roughness = glm::clamp(material->roughness, 0.05f, 0.95f);

            auto transform = glm::translate(Vector3f(x, y, 0.0f));
            auto sphere = scene->CreateSphereEntity(name, material_id, 0.5f, transform);
            scene->AttachChild(sphere, world);
        }
    }

#if 0
    {
        auto sky_light = scene->CreateEnvironmentEntity("sky_light", "@res://images/ibl/circus.hdr");
        scene->AttachChild(sky_light, root);
    }
#endif

    return scene;
}

}  // namespace my
