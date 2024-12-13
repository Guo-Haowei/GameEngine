#include "engine/core/framework/entry_point.h"
#include "engine/core/string/string_utils.h"

namespace my {

extern Scene* CreateTheAviatorScene();

class Game : public Application {
public:
    Game(const ApplicationSpec& p_spec) : Application(p_spec) {
        m_state = Application::State::SIM;
    }

    Scene* CreateInitialScene() override {
        Scene* scene = CreateTheAviatorScene();
        m_activeScene = scene;
        return scene;
    }
};

Application* CreateApplication() {
    std::string_view root = StringUtils::BasePath(__FILE__);
    root = StringUtils::BasePath(root);

    ApplicationSpec spec{};
    spec.rootDirectory = root;
    spec.name = "TheAviator";
    spec.width = 800;
    spec.height = 600;
    spec.backend = Backend::EMPTY;
    spec.decorated = true;
    spec.fullscreen = false;
    spec.vsync = false;
    spec.enableImgui = false;
    return new Game(spec);
}

}  // namespace my

int main(int p_argc, const char** p_argv) {
    return my::Main(p_argc, p_argv);
}
