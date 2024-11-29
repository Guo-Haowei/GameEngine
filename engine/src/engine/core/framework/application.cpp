#include "application.h"

#include <imgui/imgui.h>

#include "engine/core/debugger/profiler.h"
#include "engine/core/dynamic_variable/dynamic_variable_manager.h"
#include "engine/core/framework/asset_manager.h"
#include "engine/core/framework/common_dvars.h"
#include "engine/core/framework/display_manager.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/imgui_manager.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/layer.h"
#include "engine/core/framework/physics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/os/threads.h"
#include "engine/core/os/timer.h"
#include "engine/core/string/string_utils.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/render_manager.h"

#define DEFINE_DVAR
#include "engine/core/framework/common_dvars.h"
#undef DEFINE_DVAR

namespace my {

namespace fs = std::filesystem;

// @TODO: refactor
static constexpr const char* DVAR_CACHE_FILE = "@user://dynamic_variables.cache";

static void RegisterCommonDvars() {
#define REGISTER_DVAR
#include "engine/core/framework/common_dvars.h"
#undef REGISTER_DVAR
}

Application::Application(const ApplicationSpec& p_spec) : m_specification(p_spec) {
    // select work directory
    if (m_specification.rootDirectory.empty()) {
        LOG_ERROR("root directory is not set");
    } else {
        LOG_OK("root directory is '{}'", m_specification.rootDirectory);
    }

    m_userFolder = std::string{ m_specification.rootDirectory };
    m_userFolder.append(DELIMITER_STR "user");

    m_resourceFolder = std::string{ m_specification.rootDirectory };
    m_resourceFolder.append(DELIMITER_STR "resources");

    FileAccess::SetFolderCallback(
        [&]() { return m_userFolder.c_str(); },
        [&]() { return m_resourceFolder.c_str(); });
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
    RegisterModule(m_displayServer.get());
    RegisterModule(m_graphicsManager.get());
    RegisterModule(m_renderManager.get());
    RegisterModule(m_inputManager.get());

    if (m_specification.enableImgui) {
        m_imguiManager = std::make_shared<ImguiManager>();
        RegisterModule(m_imguiManager.get());
    }

    m_eventQueue.RegisterListener(m_graphicsManager.get());
    m_eventQueue.RegisterListener(m_physicsManager.get());

    return Result<void>();
}

auto Application::Initialize(int p_argc, const char** p_argv) -> Result<void> {
    SaveCommandLine(p_argc, p_argv);
    RegisterCommonDvars();
    // @TODO: refactor this part
    renderer::register_rendering_dvars();
    DynamicVariableManager::Deserialize(DVAR_CACHE_FILE);
    // parse happens after deserialization, so command line will override cache
    DynamicVariableManager::Parse(m_commandLine);

    // select window size
    {
        const Vector2i resolution{ DVAR_GET_IVEC2(window_resolution) };
        const Vector2i max_size{ 3840, 2160 };  // 4K
        const Vector2i min_size{ 480, 360 };    // 360P
        Vector2i desired_size;
        if (resolution.x > 0 && resolution.y > 0) {
            desired_size = resolution;
        } else {
            desired_size = Vector2i(m_specification.width, m_specification.height);
        }
        desired_size = glm::clamp(desired_size, min_size, max_size);
        m_specification.width = desired_size.x;
        m_specification.height = desired_size.y;
    }

    // select backend
    {
        const std::string& backend = DVAR_GET_STRING(gfx_backend);
        if (!backend.empty()) {
            do {
#define BACKEND_DECLARE(ENUM, STR, DVAR)         \
    if (backend == #DVAR) {                      \
        m_specification.backend = Backend::ENUM; \
        break;                                   \
    }
                BACKEND_LIST
#undef BACKEND_DECLARE
                return HBN_ERROR(ErrorCode::ERR_INVALID_PARAMETER, "Unkown backend '{}', set to 'empty'", backend);
            } while (0);
        }
    }

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

    InitLayers();
    for (auto& layer : m_layers) {
        layer->m_app = this;
        layer->OnAttach();
        LOG("[Runtime] layer '{}' attached!", layer->GetName());
    }

    return Result<void>();
}

void Application::Finalize() {
    // @TODO: fix
    auto [w, h] = m_displayServer->GetWindowSize();
    DVAR_SET_IVEC2(window_resolution, w, h);

    for (auto& layer : m_layers) {
        layer->OnDetach();
        LOG("[Runtime] layer '{}' detached!", layer->GetName());
    }
    m_layers.clear();

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

    DynamicVariableManager::Serialize(DVAR_CACHE_FILE);
}

void Application::Run() {
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

        if (m_imguiManager) {
            m_imguiManager->BeginFrame();
            for (auto& layer : m_layers) {
                layer->OnUpdate(dt);
            }

            for (auto& layer : m_layers) {
                layer->OnImGuiRender();
            }
            ImGui::Render();
        }

        m_physicsManager->Update(scene);
        m_graphicsManager->Update(scene);

        m_inputManager->EndFrame();
    }

    LOG("\n********************************************************************************"
        "\nMain Loop"
        "\n********************************************************************************");
}

}  // namespace my
