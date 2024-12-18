#include "editor/editor_layer.h"
#include "engine/core/framework/entry_point.h"
#include "engine/core/string/string_utils.h"

// @TODO: fix
#include "plugins/the_aviator/the_aviator_layer.h"

namespace my {

extern Scene* CreateTheAviatorScene();
extern Scene* CreatePbrTestScene();
extern Scene* CreatePhysicsTestScene();

enum {
    CREATE_EMPTY_SCENE = 0,
    CREATE_THE_AVIATOR_SCENE = 1,
    CREATE_PHYSICS_SCENE = 2,
    CREATE_PBR_SCENE = 3,

    DEFAULT_SCENE = 0,
};

class Editor : public Application {
public:
    using Application::Application;

    void InitLayers() override {
        m_editorLayer = std::make_unique<EditorLayer>();
        AttachLayer(m_editorLayer.get());

        // Only creates game layer, don't attach yet
        if constexpr (DEFAULT_SCENE == CREATE_THE_AVIATOR_SCENE) {
            m_gameLayer = std::make_unique<TheAviatorLayer>();
        }
    }

    Scene* CreateInitialScene() override;

private:
    std::unique_ptr<EditorLayer> m_editorLayer;
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

Scene* Editor::CreateInitialScene() {
    if constexpr (DEFAULT_SCENE == CREATE_EMPTY_SCENE) {
        return Application::CreateInitialScene();
    } else if constexpr (DEFAULT_SCENE == CREATE_THE_AVIATOR_SCENE) {
        return CreateTheAviatorScene();
    } else if constexpr (DEFAULT_SCENE == CREATE_PBR_SCENE) {
        return CreatePbrTestScene();
    } else if constexpr (DEFAULT_SCENE == CREATE_PHYSICS_SCENE) {
        return CreatePhysicsTestScene();
    }
}

}  // namespace my

int main(int p_argc, const char** p_argv) {
    return my::Main(p_argc, p_argv);
}
