#include "editor_layer.h"

class Editor : public my::Application {
public:
    void initLayers() override {
        addLayer(std::make_shared<my::EditorLayer>());
    }
};

int main(int p_argc, const char** p_argv) {
    Editor editor;
    return editor.run(p_argc, p_argv);
}
