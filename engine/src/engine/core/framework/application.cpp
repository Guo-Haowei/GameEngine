#include "application.h"

#include "engine/core/debugger/profiler.h"
#include "engine/core/dynamic_variable/dynamic_variable_manager.h"
#include "engine/core/framework/asset_manager.h"
#include "engine/core/framework/common_dvars.h"
#include "engine/core/framework/display_manager.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/layer.h"
#include "engine/core/framework/physics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/os/threads.h"
#include "engine/core/os/timer.h"
#include "engine/core/string/string_utils.h"
#include "engine/rendering/graphics_dvars.h"
#include "engine/rendering/render_manager.h"
#include "imgui/imgui.h"

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

    if (m_specification.enableImgui) {
        if (auto res = InitializeImgui(); !res) {
            return HBN_ERROR(res.error());
        }
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

    if (m_specification.enableImgui) {
        FinalizeImgui();
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

        // to avoid empty renderer crash
        if (m_specification.enableImgui) {
            ImGui::NewFrame();
        }
        for (auto& layer : m_layers) {
            layer->OnUpdate(dt);
        }

        for (auto& layer : m_layers) {
            layer->OnImGuiRender();
        }
        if (m_specification.enableImgui) {
            ImGui::Render();
        }

        m_physicsManager->Update(scene);
        m_graphicsManager->Update(scene);

        m_displayServer->Present();

        m_inputManager->EndFrame();
    }

    LOG("\n********************************************************************************"
        "\nMain Loop"
        "\n********************************************************************************");
}

auto Application::InitializeImgui() -> Result<void> {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    std::string_view engine_folder = StringUtils::BasePath(__FILE__);
    engine_folder = StringUtils::BasePath(engine_folder);
    engine_folder = StringUtils::BasePath(engine_folder);
    engine_folder = StringUtils::BasePath(engine_folder);
    engine_folder = StringUtils::BasePath(engine_folder);

    fs::path font_path{ engine_folder };
    font_path = font_path / "fonts" / "DroidSans.ttf";

    auto res = AssetManager::GetSingleton().LoadFileSync(FilePath(font_path.string()));
    if (!res) {
        return HBN_ERROR(res.error());
    }

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    // IM_ASSERT(font != NULL);

    auto asset = *res;
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    ImFont* font = io.Fonts->AddFontFromMemoryTTF(asset->buffer.data(), (int)asset->buffer.size(), 16, &font_cfg);
    if (!font) {
        return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "Failed to create font '{}'", font_path.string());
    }

    fs::path ini_path = fs::path{ m_userFolder } / "imgui.ini";
    m_imguiSettingsPath = ini_path.string();
    LOG_VERBOSE("imgui settings path is '{}'", m_imguiSettingsPath);
    io.IniFilename = m_imguiSettingsPath.c_str();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    auto& style = ImGui::GetStyle();
    auto& colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.105f, 0.11f, 1.0f);

    // Headers
    colors[ImGuiCol_Header] = ImVec4(0.2f, 0.205f, 0.21f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.305f, 0.31f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.2f, 0.205f, 0.21f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.305f, 0.31f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.205f, 0.21f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.305f, 0.31f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.3805f, 0.381f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.2805f, 0.281f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2f, 0.205f, 0.21f, 1.0f);

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    return Result<void>();
}

void Application::FinalizeImgui() {
    ImGui::DestroyContext();
}

}  // namespace my
