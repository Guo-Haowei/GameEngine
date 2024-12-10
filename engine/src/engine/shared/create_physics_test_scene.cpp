#include "engine/core/base/random.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/scene/scene.h"

namespace my {

/*
@TODO
    * draw array vs draw elements
    * refactor mesh
*/

Scene* CreatePhysicsTestScene() {
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
        auto point_light = scene->CreatePointLightEntity("point_light", Vector3f(0, 5, 3));
        scene->AttachChild(point_light, root);
    }

    auto world = scene->CreateTransformEntity("world");
    scene->AttachChild(world, root);

    ecs::Entity material_id = scene->CreateMaterialEntity("material");

    {
        Vector3f ground_scale(5.0f, 0.1f, 5.0f);
        auto ground = scene->CreateCubeEntity("ground_left", material_id, ground_scale);
        scene->AttachChild(ground, root);
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
        scene->AttachChild(ground, root);
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
        int x = (t - 1) % 7;
        int y = (t - 1) / 7;

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
        scene->AttachChild(id, root);
    }

    {
        auto sky_light = scene->CreateHemisphereLightEntity("sky_light", "@res://images/ibl/sky.hdr");
        scene->AttachChild(sky_light, root);
    }

    return scene;
}

}  // namespace my
