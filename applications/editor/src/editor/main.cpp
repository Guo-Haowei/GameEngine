#include "editor/editor_layer.h"
#include "engine/core/framework/entry_point.h"

namespace my {

class Editor : public Application {
public:
    using Application::Application;

    void InitLayers() override {
        AddLayer(std::make_shared<my::EditorLayer>());
    }
};

Application* CreateApplication() {
    ApplicationSpec spec{};
    spec.workDirectory;
    spec.name = "Editor";
    spec.width = 800;
    spec.height = 0;
    spec.backend = Backend::EMPTY;
    spec.decorated = true;
    spec.fullscreen = false;
    spec.vsync = false;
    spec.enableImgui = true;
    return new Editor(spec);
}

}  // namespace my

int main(int p_argc, const char** p_argv) {
    return my::Main(p_argc, p_argv);
}
