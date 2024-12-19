#include <imgui/imgui_internal.h>

#include "engine/core/framework/common_dvars.h"
#include "engine/core/framework/entry_point.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/layer.h"
#include "engine/core/string/string_utils.h"
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
    }

    void OnImGuiRender() override {
        ImGui::GetMainViewport();

        static bool opt_padding = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |=
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
            window_flags |= ImGuiWindowFlags_NoBackground;
        }

        if (!opt_padding) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        }
        ImGui::Begin("DockSpace Demo", nullptr, window_flags);
        if (!opt_padding) {
            ImGui::PopStyleVar();
        }

        ImGui::PopStyleVar(2);

        // Submit the DockSpace
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        {
            ImGui::Begin("Canvas");
            auto texture = GraphicsManager::GetSingleton().FindTexture(RESOURCE_TONE);
            if (texture) {
                auto handle = texture->GetHandle();

                ImVec2 size;
                size.x = (float)texture->desc.width;
                size.y = (float)texture->desc.height;

                switch (GraphicsManager::GetSingleton().GetBackend()) {
                    case Backend::OPENGL:
                        ImGui::Image((ImTextureID)handle, size, ImVec2(0, 1), ImVec2(1, 0));
                        break;
                    default:
                        ImGui::Image((ImTextureID)handle, size);
                        break;
                }
            }
            ImGui::End();
        }

        ImGui::End();
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
    // spec.enableImgui = false;
    spec.enableImgui = true;

    DVAR_window_resolution.SetVector2i(1600, 900);
    DVAR_window_resolution.SetFlag(DVAR_FLAG_OVERRIDEN);
    return new Game(spec);
}

}  // namespace my

int main(int p_argc, const char** p_argv) {
    return my::Main(p_argc, p_argv);
}
