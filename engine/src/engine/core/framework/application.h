#pragma once
#include "engine/core/framework/event_queue.h"
#include "engine/core/framework/layer.h"
#include "engine/core/framework/module.h"
#include "engine/rendering/graphics_defines.h"

namespace my {

class AssetManager;
class DisplayManager;
class GraphicsManager;
class ImGuiModule;
class InputManager;
class PhysicsManager;
class RenderManager;
class SceneManager;

struct ApplicationSpec {
    std::string workDirectory;
    std::string_view name;
    int width;
    int height;
    Backend backend;
    bool decorated;
    bool fullscreen;
    bool vsync;
    bool enableImgui;
};

class Application {
public:
    Application(const ApplicationSpec& p_spec);

    auto Initialize(int p_argc, const char** p_argv) -> Result<void>;
    void Finalize();
    void Run();

    virtual void InitLayers() {}

    EventQueue& GetEventQueue() { return m_eventQueue; }

    AssetManager* GetAssetManager() { return m_assetManager.get(); }
    InputManager* GetInputManager() { return m_inputManager.get(); }
    SceneManager* GetSceneManager() { return m_sceneManager.get(); }
    PhysicsManager* GetPhysicsManager() { return m_physicsManager.get(); }
    DisplayManager* GetDisplayServer() { return m_displayServer.get(); }
    GraphicsManager* GetGraphicsManager() { return m_graphicsManager.get(); }

    const ApplicationSpec& GetSpecification() const { return m_specification; }

protected:
    void AddLayer(std::shared_ptr<Layer> p_layer);

private:
    [[nodiscard]] auto SetupModules() -> Result<void>;

    void SaveCommandLine(int p_argc, const char** p_argv);
    void RegisterModule(Module* p_module);

    std::vector<std::shared_ptr<Layer>> m_layers;
    std::vector<std::string> m_commandLine;
    std::string m_appName;
    std::string m_workingDirectory;
    ApplicationSpec m_specification;

    EventQueue m_eventQueue;

    std::shared_ptr<AssetManager> m_assetManager;
    std::shared_ptr<SceneManager> m_sceneManager;
    std::shared_ptr<PhysicsManager> m_physicsManager;
    std::shared_ptr<DisplayManager> m_displayServer;
    std::shared_ptr<GraphicsManager> m_graphicsManager;
    // @TODO: remove render manager
    std::shared_ptr<RenderManager> m_renderManager;
    std::shared_ptr<ImGuiModule> m_imguiModule;
    std::shared_ptr<InputManager> m_inputManager;

    std::vector<Module*> m_modules;
};

}  // namespace my