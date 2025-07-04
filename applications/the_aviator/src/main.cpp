#include <imgui/imgui_internal.h>

#include <filesystem>

#include "engine/core/string/string_utils.h"
#include "engine/renderer/base_graphics_manager.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/renderer.h"
#include "engine/runtime/common_dvars.h"
#include "engine/runtime/entry_point.h"
#include "engine/runtime/layer.h"

namespace my {

namespace fs = std::filesystem;

extern Scene* CreateTheAviatorScene();

// @TODO: stop using ImGui for rendering final image
class TheAviatorGame : public GameLayer {
public:
    TheAviatorGame() : GameLayer("TheAviatorGame") {}

    void OnUpdate(float p_timestep) override {
        unused(p_timestep);

        auto graphics_manager = m_app->GetGraphicsManager();
        auto resolution = DVAR_GET_IVEC2(resolution);
        if (auto image = graphics_manager->FindTexture(RESOURCE_TONE); image) {
            renderer::AddImage2D(image.get(), Vector2f(resolution));
        }
    }
};

class Game : public Application {
public:
    Game(const ApplicationSpec& p_spec) : Application(p_spec) {
        m_state = Application::State::SIM;

        m_gameLayer = std::make_unique<TheAviatorGame>();
        m_layers.emplace_back(m_gameLayer.get());
    }

    Scene* CreateInitialScene() override {
        Scene* scene = CreateTheAviatorScene();
        m_activeScene = scene;
        return scene;
    }

    std::unique_ptr<Layer> m_renderLayer;
};

Application* CreateApplication() {
    auto res_path = fs::path{ ROOT_FOLDER } / "tools/editor/resources";
    auto res_string = res_path.string();
    auto user_path = fs::path{ ROOT_FOLDER } / "applications/the_aviator/user";
    auto user_string = user_path.string();

    ApplicationSpec spec{};
    spec.resourceFolder = res_string;
    spec.userFolder = user_string;
    spec.name = "TheAviator";
    spec.width = 1600;
    spec.height = 900;
    spec.backend = Backend::OPENGL;
    spec.decorated = true;
    spec.fullscreen = false;
    spec.vsync = false;
    spec.enableImgui = false;

    DVAR_window_resolution.SetVector2i(1600, 900);
    DVAR_window_resolution.SetFlag(DVAR_FLAG_OVERRIDEN);
    return new Game(spec);
}

}  // namespace my

int main(int p_argc, const char** p_argv) {
    return my::Main(p_argc, p_argv);
}
