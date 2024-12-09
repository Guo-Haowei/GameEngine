#include "engine/renderer/graphics_dvars.h"
#include "engine/scene/scene.h"

namespace my {

/*
@TODO
    * draw array vs draw elements
    * dynamic buffer
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
        camera->SetEditor();
        scene->AttachChild(editor_camera, root);
    }
    // main camera
    {
        auto main_camera = scene->CreatePerspectiveCameraEntity("main_camera", frame_size.x, frame_size.y);
        auto camera = scene->GetComponent<PerspectiveCameraComponent>(main_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0, 5, 12));
        camera->SetMain();
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
        Vector3f scale(5.0f, 0.1f, 5.0f);
        auto ground = scene->CreateCubeEntity("Ground", material_id, scale);
        scene->AttachChild(ground, root);
        auto& rigid_body = scene->Create<RigidBodyComponent>(ground);
        rigid_body.shape = RigidBodyComponent::SHAPE_CUBE;
        rigid_body.param.box.half_size = scale;
        rigid_body.mass = 0.0f;
    }

    {
        constexpr float h = 2.0f;
        Vector3f p0(-4.0f, h, +4.0f);  // top left
        Vector3f p1(+4.0f, h, +4.0f);  // top right
        Vector3f p2(+4.0f, h, -4.0f);  // bottom right
        Vector3f p3(-4.0f, h, -4.0f);  // bottom left

        Matrix4x4f transform = glm::translate(Vector3f(0, 4, 0));
        auto cloth = scene->CreateClothEntity("cloth", material_id, p0, p1, p2, p3, Vector2i(30), 15, transform);
        scene->AttachChild(cloth, root);
    }

    for (int t = 1; t <= 21; ++t) {
        int x = (t - 1) % 7;
        int y = (t - 1) / 7;

        Vector3f translate(x - 3, 6 - y, 0);
        Vector3f scale(0.25f);

        ecs::Entity id;
        if (t % 2) {
            id = scene->CreateCubeEntity(std::format("Cube_{}", t), material_id, scale, glm::translate(translate));
            auto& rigid_body = scene->Create<RigidBodyComponent>(id);
            rigid_body.shape = RigidBodyComponent::SHAPE_CUBE;
            rigid_body.param.box.half_size = scale;
        } else {
            id = scene->CreateSphereEntity(std::format("Sphere_{}", t), material_id, 0.25f, glm::translate(translate));
            auto& rigid_body = scene->Create<RigidBodyComponent>(id);
            rigid_body.shape = RigidBodyComponent::SHAPE_SPHERE;
            rigid_body.param.sphere.radius = 0.25f;
        }
        scene->AttachChild(id, root);
    }

    {
        auto sky_light = scene->CreateHemisphereLightEntity("sky_light", "@res://images/ibl/circus.hdr");
        scene->AttachChild(sky_light, root);
    }

    return scene;
}

}  // namespace my
