#pragma once
#include "engine/core/base/noncopyable.h"
#include "engine/core/os/timer.h"
#include "engine/renderer/graphics_defines.h"
#include "engine/runtime/event_queue.h"
#include "engine/runtime/layer.h"
#include "engine/runtime/module.h"

namespace my {

class AssetManager;
class AssetRegistry;
class CameraComponent;
class DisplayManager;
class IGraphicsManager;
class ImguiManager;
class InputManager;
class IPhysicsManager;
class RenderSystem;
class Scene;
class SceneManager;
class ScriptManager;

struct ApplicationSpec {
    std::string_view resourceFolder;
    std::string_view userFolder;
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

    enum class Type : uint32_t {
        RUNTIME,
        EDITOR,
    };

    Application(const ApplicationSpec& p_spec, Type p_type = Type::RUNTIME);
    virtual ~Application() = default;

    auto Initialize(int p_argc, const char** p_argv) -> Result<void>;
    void Finalize();
    static void Run(Application* p_app);

    void AttachLayer(Layer* p_layer);
    void DetachLayer(Layer* p_layer);
    void AttachGameLayer();
    void DetachGameLayer();

    EventQueue& GetEventQueue() { return m_eventQueue; }

    AssetRegistry* GetAssetRegistry() { return m_assetRegistry; }
    AssetManager* GetAssetManager() { return m_assetManager; }
    InputManager* GetInputManager() { return m_inputManager; }
    SceneManager* GetSceneManager() { return m_sceneManager; }
    IPhysicsManager* GetPhysicsManager() { return m_physicsManager; }
    ScriptManager* GetScriptManager() { return m_scriptManager; }
    DisplayManager* GetDisplayServer() { return m_displayServer; }
    IGraphicsManager* GetGraphicsManager() { return m_graphicsManager; }
    ImguiManager* GetImguiManager() { return m_imguiManager; }
    RenderSystem* GetRenderSystem() { return m_renderSystem; }

    const ApplicationSpec& GetSpecification() const { return m_specification; }
    const std::string& GetUserFolder() const { return m_userFolder; }
    const std::string& GetResourceFolder() const { return m_resourceFolder; }

    Scene* GetActiveScene() const { return m_activeScene; }
    void SetActiveScene(Scene* p_scene) { m_activeScene = p_scene; }

    State GetState() const { return m_state; }
    void SetState(State p_state);

    bool IsRuntime() const { return m_type == Type::RUNTIME; }
    bool IsEditor() const { return m_type == Type::EDITOR; }

    virtual Scene* CreateInitialScene();

    virtual CameraComponent* GetActiveCamera() = 0;

protected:
    [[nodiscard]] auto SetupModules() -> Result<void>;

    bool MainLoop();

    float UpdateTime();

    virtual void RegisterDvars();
    virtual void InitLayers() {}
    // @TODO: add CreateXXXManager for all managers
    virtual Result<ImguiManager*> CreateImguiManager();
    virtual Result<ScriptManager*> CreateScriptManager();

    void SaveCommandLine(int p_argc, const char** p_argv);
    void RegisterModule(Module* p_module);

    std::unique_ptr<GameLayer> m_gameLayer;
    std::vector<Layer*> m_layers;

    std::vector<std::string> m_commandLine;
    std::string m_appName;
    std::string m_userFolder;
    std::string m_resourceFolder;
    ApplicationSpec m_specification;

    EventQueue m_eventQueue;

    AssetRegistry* m_assetRegistry{ nullptr };
    AssetManager* m_assetManager{ nullptr };
    SceneManager* m_sceneManager{ nullptr };
    IPhysicsManager* m_physicsManager{ nullptr };
    DisplayManager* m_displayServer{ nullptr };
    IGraphicsManager* m_graphicsManager{ nullptr };
    ImguiManager* m_imguiManager{ nullptr };
    ScriptManager* m_scriptManager{ nullptr };
    RenderSystem* m_renderSystem{ nullptr };
    InputManager* m_inputManager{ nullptr };

    std::vector<Module*> m_modules;

    const Type m_type;
    State m_state{ State::EDITING };
    Scene* m_activeScene{ nullptr };

    Timer m_timer;
};

}  // namespace my
