#pragma once
#include "core/framework/event_queue.h"
#include "core/framework/layer.h"
#include "core/framework/module.h"
#include "core/os/os.h"

namespace my {

class AssetManager;
class DisplayManager;
class GraphicsManager;
class ImGuiModule;
class PhysicsManager;
class RenderManager;
class SceneManager;

class Application {
public:
    int run(int argc, const char** argv);

    virtual void initLayers(){};

    EventQueue& getEventQueue() { return m_event_queue; }

protected:
    void addLayer(std::shared_ptr<Layer> layer);

private:
    void saveCommandLine(int argc, const char** argv);

    void registerModule(Module* module);
    void setupModules();

    std::vector<std::shared_ptr<Layer>> m_layers;
    std::vector<std::string> m_command_line;
    std::string m_app_name;

    std::shared_ptr<OS> m_os;

    EventQueue m_event_queue;

    std::shared_ptr<AssetManager> m_asset_manager;
    std::shared_ptr<SceneManager> m_scene_manager;
    std::shared_ptr<PhysicsManager> m_physics_manager;
    std::shared_ptr<DisplayManager> m_display_server;
    std::shared_ptr<GraphicsManager> m_graphics_manager;
    std::shared_ptr<RenderManager> m_render_manager;
    std::shared_ptr<ImGuiModule> m_imgui_module;

    std::vector<Module*> m_modules;
};

}  // namespace my