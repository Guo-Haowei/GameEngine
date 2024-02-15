#include "editor_layer.h"

class Editor : public my::Application {
public:
    void init_layers() override {
        add_layer(std::make_shared<my::EditorLayer>());
    }
};

int main(int argc, const char** argv) {
    Editor editor;
    return editor.run(argc, argv);
}
