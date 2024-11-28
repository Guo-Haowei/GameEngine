#include "engine/core/framework/entry_point.h"

#include "editor/editor_layer.h"

class Editor : public my::Application {
public:
    void InitLayers() override {
        AddLayer(std::make_shared<my::EditorLayer>());
    }
};

my::Application* CreateApplication() {
    return new Editor();
}

int main(int p_argc, const char** p_argv) {
    return my::Main(p_argc, p_argv);
}

