#pragma once
#include "engine/core/framework/event_queue.h"
#include "engine/core/framework/module.h"
#include "engine/renderer/graphics_defines.h"

namespace my {

class AssetManager;
class DisplayManager;
class GraphicsManager;
class ImguiManager;
class InputManager;
class PhysicsManager;
class RenderManager;
class SceneManager;

class Layer;

struct ApplicationSpec {
    std::string_view rootDirectory;
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
    virtual ~Application() = default;

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
    ImguiManager* GetImguiManager() { return m_imguiManager.get(); }

    const ApplicationSpec& GetSpecification() const { return m_specification; }
    const std::string& GetUserFolder() const { return m_userFolder; }

protected:
    void AddLayer(std::shared_ptr<Layer> p_layer);

private:
    [[nodiscard]] auto SetupModules() -> Result<void>;

    void SaveCommandLine(int p_argc, const char** p_argv);
    void RegisterModule(Module* p_module);

    std::vector<std::shared_ptr<Layer>> m_layers;
    std::vector<std::string> m_commandLine;
    std::string m_appName;
    std::string m_userFolder;
    std::string m_resourceFolder;
    ApplicationSpec m_specification;

    EventQueue m_eventQueue;

    std::shared_ptr<AssetManager> m_assetManager;
    std::shared_ptr<SceneManager> m_sceneManager;
    std::shared_ptr<PhysicsManager> m_physicsManager;
    std::shared_ptr<DisplayManager> m_displayServer;
    std::shared_ptr<GraphicsManager> m_graphicsManager;
    std::shared_ptr<ImguiManager> m_imguiManager;
    // @TODO: remove render manager
    std::shared_ptr<RenderManager> m_renderManager;
    std::shared_ptr<InputManager> m_inputManager;

    std::vector<Module*> m_modules;
};

}  // namespace my
