#pragma once
#include "engine/core/base/noncopyable.h"
#include "engine/core/framework/event_queue.h"
#include "engine/core/framework/module.h"
#include "engine/renderer/graphics_defines.h"

namespace my {

class AssetManager;
class AssetRegistry;
class DisplayManager;
class GraphicsManager;
class ImguiManager;
class InputManager;
class PhysicsManager;
class RenderManager;
class SceneManager;
class ScriptManager;

class Layer;
class Scene;

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

class Application : public NonCopyable {
public:
    enum class State : uint8_t {
        EDITING,
        SIM,
        BEGIN_SIM,
        END_SIM,
        COUNT,
    };

    Application(const ApplicationSpec& p_spec);
    virtual ~Application() = default;

    auto Initialize(int p_argc, const char** p_argv) -> Result<void>;
    void Finalize();
    void Run();

    virtual void InitLayers() {}

    void AttachLayer(Layer* p_layer);
    void DetachLayer(Layer* p_layer);
    void AttachGameLayer();
    void DetachGameLayer();

    EventQueue& GetEventQueue() { return m_eventQueue; }

    AssetRegistry* GetAssetRegistry() { return m_assetRegistry; }
    AssetManager* GetAssetManager() { return m_assetManager; }
    InputManager* GetInputManager() { return m_inputManager; }
    SceneManager* GetSceneManager() { return m_sceneManager; }
    PhysicsManager* GetPhysicsManager() { return m_physicsManager; }
    DisplayManager* GetDisplayServer() { return m_displayServer; }
    GraphicsManager* GetGraphicsManager() { return m_graphicsManager; }
    ImguiManager* GetImguiManager() { return m_imguiManager; }

    const ApplicationSpec& GetSpecification() const { return m_specification; }
    const std::string& GetUserFolder() const { return m_userFolder; }
    const std::string& GetResourceFolder() const { return m_resourceFolder; }

    Scene* GetActiveScene() const { return m_activeScene; }
    void SetActiveScene(Scene* p_scene) { m_activeScene = p_scene; }

    State GetState() const { return m_state; }
    void SetState(State p_state);

    virtual Scene* CreateInitialScene();

protected:
    [[nodiscard]] auto SetupModules() -> Result<void>;

    void SaveCommandLine(int p_argc, const char** p_argv);
    void RegisterModule(Module* p_module);

    std::unique_ptr<Layer> m_gameLayer;
    std::vector<Layer*> m_layers;

    std::vector<std::string> m_commandLine;
    std::string m_appName;
    std::string m_userFolder;
    std::string m_resourceFolder;
    ApplicationSpec m_specification;

    EventQueue m_eventQueue;

    AssetRegistry* m_assetRegistry;
    AssetManager* m_assetManager;
    SceneManager* m_sceneManager;
    PhysicsManager* m_physicsManager;
    DisplayManager* m_displayServer;
    GraphicsManager* m_graphicsManager;
    ImguiManager* m_imguiManager;
    ScriptManager* m_scriptManager;
    // @TODO: remove render manager
    RenderManager* m_renderManager;
    InputManager* m_inputManager;

    std::vector<Module*> m_modules;

    Scene* m_activeScene{ nullptr };
    State m_state{ State::EDITING };
};

}  // namespace my
