#include "application.h"

#include <imgui/imgui.h>

#include "engine/core/debugger/profiler.h"
#include "engine/core/dynamic_variable/dynamic_variable_manager.h"
#include "engine/core/framework/asset_manager.h"
#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/common_dvars.h"
#include "engine/core/framework/display_manager.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/imgui_manager.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/layer.h"
#include "engine/core/framework/physics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/framework/script_manager.h"
#include "engine/core/math/vector_math.h"
#include "engine/core/os/threads.h"
#include "engine/core/os/timer.h"
#include "engine/core/string/string_utils.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/renderer.h"
#include "engine/scene/scene.h"

#define DEFINE_DVAR
#include "engine/core/framework/common_dvars.h"
#undef DEFINE_DVAR

// @TODO: remove this
#include "engine/renderer/render_manager.h"

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

void Application::AttachLayer(Layer* p_layer) {
    DEV_ASSERT(p_layer);

    p_layer->m_app = this;
    p_layer->OnAttach();
    m_layers.emplace_back(p_layer);
}

void Application::DetachLayer(Layer* p_layer) {
    DEV_ASSERT(p_layer);

    auto it = std::find(m_layers.begin(), m_layers.end(), p_layer);
    if (it == m_layers.end()) {
        LOG_WARN("Layer '{}' not found");
        return;
    }

    m_layers.erase(it);
    p_layer->OnDetach();
    p_layer->m_app = nullptr;
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
    m_assetManager = new AssetManager();
    m_assetRegistry = new AssetRegistry();
    m_scriptManager = new ScriptManager();
    m_sceneManager = new SceneManager();
    m_physicsManager = new PhysicsManager();
    m_displayServer = DisplayManager::Create();
    {
        auto res = GraphicsManager::Create();
        if (!res) {
            return HBN_ERROR(res.error());
        }
        m_graphicsManager = *res;
    }
    m_renderManager = new RenderManager();
    m_inputManager = new InputManager();

    RegisterModule(m_assetManager);
    RegisterModule(m_assetRegistry);
    RegisterModule(m_sceneManager);
    RegisterModule(m_scriptManager);
    RegisterModule(m_physicsManager);
    RegisterModule(m_displayServer);
    RegisterModule(m_graphicsManager);
    RegisterModule(m_renderManager);
    RegisterModule(m_inputManager);

    if (m_specification.enableImgui) {
        m_imguiManager = new ImguiManager;
        RegisterModule(m_imguiManager);
    }

    m_eventQueue.RegisterListener(m_graphicsManager);

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
        const NewVector2i resolution{ DVAR_GET_IVEC2(window_resolution) };
        const NewVector2i max_size{ 3840, 2160 };  // 4K
        const NewVector2i min_size{ 480, 360 };    // 360P
        NewVector2i desired_size;
        if (resolution.x > 0 && resolution.y > 0) {
            desired_size = resolution;
        } else {
            desired_size = NewVector2i(m_specification.width, m_specification.height);
        }
        desired_size = math::Clamp(desired_size, min_size, max_size);
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

    DynamicVariableManager::Serialize(DVAR_CACHE_FILE);
}

void Application::Run() {
    LOG_WARN("TODO: reverse z");

    LOG("\n********************************************************************************"
        "\nMain Loop"
        "\n********************************************************************************");

    // @TODO: add frame count, timestep time, etc
    Timer timer;
    do {
        OPTICK_FRAME("MainThread");

        m_displayServer->BeginFrame();
        if (m_displayServer->ShouldClose()) {
            break;
        }

        renderer::BeginFrame();

        m_inputManager->BeginFrame();

        // @TODO: better elapsed time
        float timestep = static_cast<float>(timer.GetDuration().ToSecond());
        timestep = glm::min(timestep, 0.5f);
        timer.Start();

        m_inputManager->GetEventQueue().FlushEvents();

        // 1. scene manager checks for update, and if it's necessary to swap scene
        // 2. renderer builds render data
        // 3. editor modifies scene
        // 4. script manager updates logic
        // 5. phyiscs manager updates physics
        // 6. graphcs manager renders (optional: on another thread)
        m_sceneManager->Update();

        // layer should set active scene
        // update layers from back to front
        for (int i = (int)m_layers.size() - 1; i >= 0; --i) {
            m_layers[i]->OnUpdate(timestep);
        }

        m_activeScene->Update(timestep);
        ecs::Entity camera;
        switch (m_state) {
            case Application::State::EDITING:
                camera = m_activeScene->GetEditorCamera();
                break;
            case Application::State::SIM:
                camera = m_activeScene->GetMainCamera();
                break;
            case Application::State::BEGIN_SIM:
                break;
            case Application::State::END_SIM:
                break;
            default:
                break;
        }
        PerspectiveCameraComponent* perspective_camera = nullptr;
        if (camera.IsValid()) {
            perspective_camera = m_activeScene->GetComponent<PerspectiveCameraComponent>(camera);
        } else {
            CRASH_NOW();
        }
        renderer::RequestScene(*perspective_camera, *m_activeScene);

        // @TODO: refactor this
        if (m_imguiManager) {
            m_imguiManager->BeginFrame();

            for (int i = (int)m_layers.size() - 1; i >= 0; --i) {
                m_layers[i]->OnImGuiRender();
            }

            ImGui::Render();
        }

        if (m_state == State::SIM) {
            m_scriptManager->Update(*m_activeScene);
            m_physicsManager->Update(*m_activeScene);
        }

        renderer::EndFrame();

        m_graphicsManager->Update(*m_activeScene);

        m_inputManager->EndFrame();
    } while (true);

    LOG("\n********************************************************************************"
        "\nMain Loop"
        "\n********************************************************************************");
}

void Application::AttachGameLayer() {
    if (m_gameLayer) {
        AttachLayer(m_gameLayer.get());
    }
}

void Application::DetachGameLayer() {
    if (m_gameLayer) {
        DetachLayer(m_gameLayer.get());
    }
}

void Application::SetState(State p_state) {
    switch (p_state) {
        case State::BEGIN_SIM: {
            if (DEV_VERIFY(m_state == State::EDITING)) {
                m_state = p_state;
            }
        } break;
        case State::END_SIM: {
            if (DEV_VERIFY(m_state == State::SIM)) {
                m_state = p_state;
            }
        } break;
        case State::SIM: {
            if (DEV_VERIFY(m_state == State::BEGIN_SIM)) {
                m_state = p_state;
            }
        } break;
        case State::EDITING: {
            if (DEV_VERIFY(m_state == State::END_SIM)) {
                m_state = p_state;
            }
        } break;
        default:
            CRASH_NOW();
            break;
    }
}

Scene* Application::CreateInitialScene() {
    ecs::Entity::SetSeed();

    Scene* scene = new Scene;

    NewVector2i frame_size = DVAR_GET_IVEC2(resolution);

    auto root = scene->CreateTransformEntity("world");
    scene->m_root = root;

    auto camera_id = scene->CreatePerspectiveCameraEntity("editor_camera", frame_size.x, frame_size.y);
    auto camera = scene->GetComponent<PerspectiveCameraComponent>(camera_id);
    DEV_ASSERT(camera);
    camera->SetPosition(Vector3f(0, 4, 10));
    camera->SetEditorCamera();
    scene->AttachChild(camera_id, root);

    return scene;
}

}  // namespace my
