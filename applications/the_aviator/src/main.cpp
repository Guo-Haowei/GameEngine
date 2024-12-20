#include <imgui/imgui_internal.h>

#include "engine/core/framework/common_dvars.h"
#include "engine/core/framework/entry_point.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/layer.h"
#include "engine/core/string/string_utils.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/renderer.h"
#include "plugins/the_aviator/the_aviator_layer.h"

namespace my {

extern Scene* CreateTheAviatorScene();

// @TODO: stop using ImGui for rendering final image
class RenderLayer : public Layer {
public:
    RenderLayer() : Layer("RenderLayer") {}

    void OnAttach() override {
    }

    void OnDetach() override {
    }

    void OnUpdate(float p_timestep) override {
        unused(p_timestep);

        auto graphics_manager = m_app->GetGraphicsManager();
        auto resolution = DVAR_GET_IVEC2(resolution);
        if (auto image = graphics_manager->FindTexture(RESOURCE_TONE); image) {
            renderer::AddImage2D(image.get(), NewVector2f(resolution));
        }
    }
};

class Game : public Application {
public:
    Game(const ApplicationSpec& p_spec) : Application(p_spec) {
        m_state = Application::State::SIM;

        m_renderLayer = std::make_unique<RenderLayer>();
        m_layers.emplace_back(m_renderLayer.get());

        m_gameLayer = std::make_unique<TheAviatorLayer>();
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
    std::string_view root = StringUtils::BasePath(__FILE__);
    root = StringUtils::BasePath(root);

    ApplicationSpec spec{};
    spec.rootDirectory = root;
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
