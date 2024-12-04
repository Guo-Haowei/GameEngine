#include "editor/editor_layer.h"
#include "engine/core/framework/entry_point.h"
#include "engine/core/string/string_utils.h"

// @TODO: refactor
#include "engine/core/framework/scene_manager.h"
#include "engine/core/math/geomath.h"
#include "engine/renderer/graphics_dvars.h"

namespace my {

class Editor : public Application {
public:
    using Application::Application;

    void InitLayers() override {
        AddLayer(std::make_shared<my::EditorLayer>());
    }

    Scene* CreateInitialScene() override;
};

Application* CreateApplication() {
    std::string_view root = StringUtils::BasePath(__FILE__);
    root = StringUtils::BasePath(root);
    root = StringUtils::BasePath(root);

    ApplicationSpec spec{};
    spec.rootDirectory = root;
    spec.name = "Editor";
    spec.width = 800;
    spec.height = 600;
    spec.backend = Backend::EMPTY;
    spec.decorated = true;
    spec.fullscreen = false;
    spec.vsync = false;
    spec.enableImgui = true;
    return new Editor(spec);
}

static Scene* CreateTheAviatorScene();

Scene* Editor::CreateInitialScene() {
    if constexpr (1) {
        return CreateTheAviatorScene();
    } else {
        return Application::CreateInitialScene();
    }
}

static Scene* CreateTheAviatorScene() {
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
        transform->RotateY(Degree(-45.f));
        transform->RotateX(Degree(-45.f));
    }

    // create plane
    {
        auto plane = scene->CreateTransformEntity("plane");
        scene->AttachChild(plane, root);
        auto& script = scene->Create<ScriptComponent>(plane);
        script.SetScript("@res://scripts/plane.lua");

        auto cube = scene->CreateCubeEntity("body");
        scene->AttachChild(cube, plane);

        const std::array<Vector3f, 8> cockpit_points = {
            Vector3f(+4.f, +2.5f, +2.5f),
            Vector3f(+4.f, +2.5f, -2.5f),
            Vector3f(+4.f, -2.5f, +2.5f),
            Vector3f(+4.f, -2.5f, -2.5f),
            Vector3f(-4.f, +1.5f, -0.5f),
            Vector3f(-4.f, +1.5f, +0.5f),
            Vector3f(-4.f, +0.5f, -0.5f),
            Vector3f(-4.f, +0.5f, +0.5f),
        };

        auto cockpit = scene->CreateNameEntity("cockpit_mesh");
        MeshComponent& mesh = scene->Create<MeshComponent>(cockpit);
        mesh = MakeCubeMesh(cockpit_points);
        //mesh.subsets[0].material_id = p_material_id;
    }

    return scene;
}

}  // namespace my

int main(int p_argc, const char** p_argv) {
    return my::Main(p_argc, p_argv);
}
