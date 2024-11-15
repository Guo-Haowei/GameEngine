#include "application.h"

#include "core/debugger/profiler.h"
#include "core/dynamic_variable/dynamic_variable_manager.h"
#include "core/framework/asset_manager.h"
#include "core/framework/common_dvars.h"
#include "core/framework/display_manager.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/imgui_module.h"
#include "core/framework/physics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/input/input.h"
#include "core/os/threads.h"
#include "core/os/timer.h"
#include "core/systems/job_system.h"
#include "imgui/imgui.h"
#include "rendering/graphics_dvars.h"
#include "rendering/render_manager.h"

#define DEFINE_DVAR
#include "core/framework/common_dvars.h"
#undef DEFINE_DVAR

namespace my {

static void RegisterCommonDvars() {
#define REGISTER_DVAR
#include "core/framework/common_dvars.h"
#undef REGISTER_DVAR
}

void Application::AddLayer(std::shared_ptr<Layer> p_layer) {
    m_layers.emplace_back(p_layer);
}

void Application::SaveCommandLine(int p_argc, const char** p_argv) {
    m_appName = p_argv[0];
    for (int i = 1; i < p_argc; ++i) {
        m_commandLine.push_back(p_argv[i]);
    }
}

void Application::RegisterModule(Module* p_module) {
    p_module->m_app = this;
    m_modules.push_back(p_module);
}

void Application::SetupModules() {
    m_assetManager = std::make_shared<AssetManager>();
    m_sceneManager = std::make_shared<SceneManager>();
    m_physicsManager = std::make_shared<PhysicsManager>();
    m_imguiModule = std::make_shared<ImGuiModule>();
    m_displayServer = DisplayManager::Create();
    m_graphicsManager = GraphicsManager::Create();
    m_renderManager = std::make_shared<RenderManager>();

    RegisterModule(m_assetManager.get());
    RegisterModule(m_sceneManager.get());
    RegisterModule(m_physicsManager.get());
    RegisterModule(m_imguiModule.get());
    RegisterModule(m_displayServer.get());
    RegisterModule(m_graphicsManager.get());
    RegisterModule(m_renderManager.get());

    m_eventQueue.RegisterListener(m_graphicsManager.get());
    m_eventQueue.RegisterListener(m_physicsManager.get());
}

int Application::Run(int p_argc, const char** p_argv) {
    SaveCommandLine(p_argc, p_argv);
    m_os = std::make_shared<OS>();

    // intialize
    OS::GetSingleton().Initialize();

    // dvars
    RegisterCommonDvars();
    renderer::register_rendering_dvars();
    DynamicVariableManager::deserialize();
    DynamicVariableManager::Parse(m_commandLine);

    SetupModules();

    thread::Initialize();
    jobsystem::Initialize();

    for (Module* module : m_modules) {
        LOG("module '{}' being initialized...", module->GetName());
        if (!module->Initialize()) {
            LOG_ERROR("Error: failed to initialize module '{}'", module->GetName());
            return 1;
        }
        LOG("module '{}' initialized\n", module->GetName());
    }

    InitLayers();
    for (auto& layer : m_layers) {
        layer->Attach();
        LOG("[Runtime] layer '{}' attached!", layer->GetName());
    }

    LOG("\n********************************************************************************"
        "\nMain Loop"
        "\n********************************************************************************");

    LOG_WARN("TODO: properly unload scene");
    LOG_WARN("TODO: make camera a component");
    LOG_WARN("TODO: refactor render graph");
    LOG_WARN("TODO: reverse z");
    LOG_WARN("TODO: cloth physics");

    LOG_VERBOSE("This is a verbose log");
    LOG("This is a log");
    LOG_OK("This is an ok log");
    LOG_WARN("This is a warning");
    LOG_ERROR("This is an error");

    // @TODO: add frame count, elapsed time, etc
    Timer timer;
    while (!DisplayManager::GetSingleton().ShouldClose()) {
        OPTICK_FRAME("MainThread");

        m_displayServer->NewFrame();

        input::BeginFrame();

        // @TODO: better elapsed time
        float dt = static_cast<float>(timer.GetDuration().ToSecond());
        dt = glm::min(dt, 0.5f);
        timer.Start();

        // to avoid empty renderer crash
        ImGui::NewFrame();
        for (auto& layer : m_layers) {
            layer->Update(dt);
        }

        for (auto& layer : m_layers) {
            layer->Render();
        }
        ImGui::Render();

        m_sceneManager->Update(dt);
        auto& scene = m_sceneManager->GetScene();

        m_physicsManager->Update(scene);
        m_graphicsManager->Update(scene);
        m_graphicsManager->Render();

        renderer::reset_need_update_env();

        m_displayServer->Present();

        input::EndFrame();
    }

    LOG("\n********************************************************************************"
        "\nMain Loop"
        "\n********************************************************************************");

    // @TODO: fix
    auto [w, h] = DisplayManager::GetSingleton().GetWindowSize();
    DVAR_SET_IVEC2(window_resolution, w, h);

    m_layers.clear();

    // @TODO: move it to request shutdown
    thread::RequestShutdown();

    // finalize

    for (int index = (int)m_modules.size() - 1; index >= 0; --index) {
        Module* module = m_modules[index];
        module->Finalize();
        LOG_VERBOSE("module '{}' finalized", module->GetName());
    }

    jobsystem::Finalize();
    thread::Finailize();

    DynamicVariableManager::Serialize();

    OS::GetSingleton().Finalize();

    return 0;
}

}  // namespace my