#include "editor_layer.h"

class Editor : public my::Application {
public:
    void InitLayers() override {
        AddLayer(std::make_shared<my::EditorLayer>());
    }
};

int main(int p_argc, const char** p_argv) {
    Editor editor;
    return editor.Run(p_argc, p_argv);
}
