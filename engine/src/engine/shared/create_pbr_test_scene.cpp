#include "engine/renderer/graphics_dvars.h"
#include "engine/scene/scene.h"

namespace my {

Scene* CreatePbrTestScene() {
    ecs::Entity::SetSeed();

    Scene* scene = new Scene;

    auto root = scene->CreateTransformEntity("root");
    scene->m_root = root;

    NewVector2i frame_size = DVAR_GET_IVEC2(resolution);
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

    {
        auto sky_light = scene->CreateHemisphereLightEntity("sky_light", "@res://images/ibl/circus.hdr");
        scene->AttachChild(sky_light, root);
    }

    return scene;
}

}  // namespace my
