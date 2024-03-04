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
#include "rendering/renderer.h"
#include "rendering/rendering_dvars.h"

#define DEFINE_DVAR
#include "core/framework/common_dvars.h"

namespace my {

static void register_common_dvars() {
#define REGISTER_DVAR
#include "core/framework/common_dvars.h"
}

void Application::add_layer(std::shared_ptr<Layer> layer) {
    m_layers.emplace_back(layer);
}

void Application::save_command_line(int argc, const char** argv) {
    m_app_name = argv[0];
    for (int i = 1; i < argc; ++i) {
        m_command_line.push_back(argv[i]);
    }
}

void Application::register_module(Module* module) {
    module->m_app = this;
    m_modules.push_back(module);
}

void Application::setup_modules() {
    m_asset_manager = std::make_shared<AssetManager>();
    m_scene_manager = std::make_shared<SceneManager>();
    m_physics_manager = std::make_shared<PhysicsManager>();
    m_imgui_module = std::make_shared<ImGuiModule>();
    m_display_server = DisplayManager::create();
    m_graphics_manager = GraphicsManager::create();

    register_module(m_asset_manager.get());
    register_module(m_scene_manager.get());
    register_module(m_physics_manager.get());
    register_module(m_imgui_module.get());
    register_module(m_display_server.get());
    register_module(m_graphics_manager.get());

    m_event_queue.register_listener(m_graphics_manager.get());
    m_event_queue.register_listener(m_physics_manager.get());
}

int Application::run(int argc, const char** argv) {
    save_command_line(argc, argv);
    m_os = std::make_shared<OS>();

    // intialize
    OS::singleton().initialize();

    // dvars
    register_common_dvars();
    renderer::register_rendering_dvars();
    DynamicVariableManager::deserialize();
    DynamicVariableManager::parse(m_command_line);

    setup_modules();

    thread::initialize();
    jobsystem::initialize();
    renderer::initialize();

    for (Module* module : m_modules) {
        LOG("module '{}' being initialized...", module->get_name());
        if (!module->initialize()) {
            LOG_ERROR("Error: failed to initialize module '{}'", module->get_name());
            return 1;
        }
        LOG("module '{}' initialized\n", module->get_name());
    }

    init_layers();
    for (auto& layer : m_layers) {
        layer->attach();
        LOG("[Runtime] layer '{}' attached!", layer->get_name());
    }

    LOG("\n********************************************************************************"
        "\nMain Loop"
        "\n********************************************************************************");

    LOG_WARN("TODO: terrain & grass");
    LOG_WARN("TODO: water");
    LOG_OK("TODO: change position buffer to view space");
    LOG_WARN("TODO: area light shadow & fix cascade shadow");
    LOG_OK("TODO: selection highlight");
    LOG_OK("TODO: TAA");
    LOG_WARN("TODO: cloud");

    LOG_WARN("TODO: properly unload scene");
    LOG_WARN("TODO: make camera a component");
    LOG_WARN("TODO: use lua to construct scene");
    LOG_WARN("TODO: refactor render graph");

    LOG_ERROR("TODO: path tracer here");
    LOG_ERROR("TODO: cloth physics");

    // @TODO: add frame count, elapsed time, etc
    Timer timer;
    while (!DisplayManager::singleton().should_close()) {
        OPTICK_FRAME("MainThread");

        m_display_server->new_frame();

        input::begin_frame();

        // @TODO: better elapsed time
        float dt = static_cast<float>(timer.get_duration().to_second());
        dt = glm::min(dt, 0.5f);
        timer.start();

        // to avoid empty renderer crash
        ImGui::NewFrame();
        for (auto& layer : m_layers) {
            layer->update(dt);
        }

        for (auto& layer : m_layers) {
            layer->render();
        }
        ImGui::Render();

        m_scene_manager->update(dt);

        m_physics_manager->update(dt);

        m_graphics_manager->update(dt);
        renderer::reset_need_update_env();

        m_display_server->present();

        input::end_frame();
    }

    LOG("\n********************************************************************************"
        "\nMain Loop"
        "\n********************************************************************************");

    // @TODO: fix
    auto [w, h] = DisplayManager::singleton().get_window_size();
    DVAR_SET_IVEC2(window_resolution, w, h);

    m_layers.clear();

    // @TODO: move it to request shutdown
    thread::request_shutdown();

    // finalize

    for (int index = (int)m_modules.size() - 1; index >= 0; --index) {
        Module* module = m_modules[index];
        module->finalize();
        LOG_VERBOSE("module '{}' finalized", module->get_name());
    }

    renderer::finalize();
    jobsystem::finalize();
    thread::finailize();

    DynamicVariableManager::serialize();

    OS::singleton().finalize();

    return 0;
}

}  // namespace my