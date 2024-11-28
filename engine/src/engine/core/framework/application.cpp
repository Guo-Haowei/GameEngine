#include "application.h"

#include "engine/core/debugger/profiler.h"
#include "engine/core/dynamic_variable/dynamic_variable_manager.h"
#include "engine/core/framework/asset_manager.h"
#include "engine/core/framework/common_dvars.h"
#include "engine/core/framework/display_manager.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/imgui_module.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/physics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/os/threads.h"
#include "engine/core/os/timer.h"
#include "engine/core/string/string_builder.h"
#include "engine/rendering/graphics_dvars.h"
#include "engine/rendering/render_manager.h"
#include "imgui/imgui.h"

#define DEFINE_DVAR
#include "engine/core/framework/common_dvars.h"
#undef DEFINE_DVAR

namespace my {

static void RegisterCommonDvars() {
#define REGISTER_DVAR
#include "engine/core/framework/common_dvars.h"
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
    DEV_ASSERT(p_module);
    p_module->m_app = this;
    m_modules.push_back(p_module);
}

auto Application::SetupModules() -> Result<void> {
    m_assetManager = std::make_shared<AssetManager>();
    m_sceneManager = std::make_shared<SceneManager>();
    m_physicsManager = std::make_shared<PhysicsManager>();
    m_imguiModule = std::make_shared<ImGuiModule>();
    m_displayServer = DisplayManager::Create();
    {
        auto res = GraphicsManager::Create();
        if (!res) {
            return HBN_ERROR(res.error());
        }
        m_graphicsManager = *res;
    }
    m_renderManager = std::make_shared<RenderManager>();
    m_inputManager = std::make_shared<InputManager>();

    RegisterModule(m_assetManager.get());
    RegisterModule(m_sceneManager.get());
    RegisterModule(m_physicsManager.get());
    RegisterModule(m_imguiModule.get());
    RegisterModule(m_displayServer.get());
    RegisterModule(m_graphicsManager.get());
    RegisterModule(m_renderManager.get());
    RegisterModule(m_inputManager.get());

    m_eventQueue.RegisterListener(m_graphicsManager.get());
    m_eventQueue.RegisterListener(m_physicsManager.get());

    return Result<void>();
}

void Application::MainLoop() {
    InitLayers();
    for (auto& layer : m_layers) {
        layer->m_app = this;
        layer->Attach();
        LOG("[Runtime] layer '{}' attached!", layer->GetName());
    }

    LOG_WARN("TODO: reverse z");

    LOG("\n********************************************************************************"
        "\nMain Loop"
        "\n********************************************************************************");

    // @TODO: add frame count, elapsed time, etc
    Timer timer;
    while (!m_displayServer->ShouldClose()) {
        OPTICK_FRAME("MainThread");

        m_displayServer->BeginFrame();

        m_inputManager->BeginFrame();

        // @TODO: better elapsed time
        float dt = static_cast<float>(timer.GetDuration().ToSecond());
        dt = glm::min(dt, 0.5f);
        timer.Start();

        m_inputManager->GetEventQueue().FlushEvents();

        m_sceneManager->Update(dt);
        auto& scene = m_sceneManager->GetScene();

        // to avoid empty renderer crash
        ImGui::NewFrame();
        for (auto& layer : m_layers) {
            layer->Update(dt);
        }

        for (auto& layer : m_layers) {
            layer->Render();
        }
        ImGui::Render();

        m_physicsManager->Update(scene);
        m_graphicsManager->Update(scene);

        renderer::reset_need_update_env();

        m_displayServer->Present();

        m_inputManager->EndFrame();
    }

    LOG("\n********************************************************************************"
        "\nMain Loop"
        "\n********************************************************************************");

    // @TODO: fix
    auto [w, h] = m_displayServer->GetWindowSize();
    DVAR_SET_IVEC2(window_resolution, w, h);

    for (auto& layer : m_layers) {
        layer->Detach();
        LOG("[Runtime] layer '{}' detached!", layer->GetName());
    }
    m_layers.clear();
}

auto Application::Setup() -> Result<void> {
    // dvars
    RegisterCommonDvars();
    renderer::register_rendering_dvars();
    DynamicVariableManager::deserialize();
    DynamicVariableManager::Parse(m_commandLine);

    if (auto res = SetupModules(); !res) {
        return HBN_ERROR(res.error());
    }

    for (Module* module : m_modules) {
        LOG("module '{}' being initialized...", module->GetName());
        if (auto res = module->Initialize(); !res) {
            // LOG_ERROR("Error: failed to initialize module '{}'", module->GetName());
            return HBN_ERROR(res.error());
        }
        LOG("module '{}' initialized\n", module->GetName());
    }

    return Result<void>();
}

int Application::Run(int p_argc, const char** p_argv) {
    SaveCommandLine(p_argc, p_argv);

    auto res = Setup();
    if (!res) {
        StringStreamBuilder builder;
        builder << res.error();
        LOG_ERROR("{}", builder.ToString());
    } else {
        MainLoop();
    }

    // @TODO: move it to request shutdown
    thread::RequestShutdown();

    for (int index = (int)m_modules.size() - 1; index >= 0; --index) {
        Module* module = m_modules[index];
        module->Finalize();
        LOG_VERBOSE("module '{}' finalized", module->GetName());
    }

#if 0
    LOG_ERROR("This is an error");
    LOG_VERBOSE("This is a verbose log");
    LOG("This is a log");
    LOG_OK("This is an ok log");
    LOG_WARN("This is a warning");
    LOG_FATAL("This is a fatal error");
#endif

    DynamicVariableManager::Serialize();

    return res ? 0 : 1;
}

}  // namespace my
