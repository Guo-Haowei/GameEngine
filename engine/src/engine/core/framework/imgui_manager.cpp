#include "imgui_manager.h"

#include <IconsFontAwesome/IconsFontAwesome6.h>
#include <imgui/imgui.h>

#include <filesystem>

#include "engine/core/framework/application.h"
#include "engine/core/framework/asset_manager.h"
#include "engine/core/framework/asset_registry.h"
#include "engine/core/string/string_utils.h"

namespace my {

namespace fs = std::filesystem;

auto ImguiManager::InitializeImpl() -> Result<void> {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    std::string_view engine_folder = StringUtils::BasePath(__FILE__);
    engine_folder = StringUtils::BasePath(engine_folder);
    engine_folder = StringUtils::BasePath(engine_folder);
    engine_folder = StringUtils::BasePath(engine_folder);
    engine_folder = StringUtils::BasePath(engine_folder);

    ImGuiIO& io = ImGui::GetIO();
    const float base_font_size = 16.0f;                         // This is the size of the default font. Change to the font size you use.
    const float icon_font_size = base_font_size * 2.0f / 3.0f;  // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

    // dummy wait here
    AssetManager::Wait();
    {
        const std::string path = "@res://fonts/DroidSans.ttf";
        auto font = m_app->GetAssetRegistry()->GetAssetByHandle<BufferAsset>(path);

        if (DEV_VERIFY(font)) {
            ImFontConfig font_cfg;
            font_cfg.FontDataOwnedByAtlas = false;

            void* data = (void*)font->buffer.data();
            if (!io.Fonts->AddFontFromMemoryTTF(data, (int)font->buffer.size(), base_font_size, &font_cfg)) {
                return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "Failed to create font '{}'", path);
            }
        }
    }

    {
        const std::string path = "@res://fonts/" FONT_ICON_FILE_NAME_FAS;
        auto font = m_app->GetAssetRegistry()->GetAssetByHandle<BufferAsset>(path);

        if (DEV_VERIFY(font)) {
            // merge in icons from Font Awesome
            static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
            ImFontConfig icons_config;
            icons_config.MergeMode = true;
            icons_config.PixelSnapH = true;
            icons_config.GlyphMinAdvanceX = icon_font_size;
            icons_config.FontDataOwnedByAtlas = false;

            void* data = (void*)font->buffer.data();
            if (!io.Fonts->AddFontFromMemoryTTF(data, (int)font->buffer.size(), base_font_size, &icons_config, icons_ranges)) {
                return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "Failed to create font '{}'", path);
            }
        }
    }

    fs::path ini_path = fs::path{ m_app->GetUserFolder() } / "imgui.ini";
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

    DEV_ASSERT(m_displayInitializeFunc);
    m_displayInitializeFunc();

    DEV_ASSERT(m_rendererInitializeFunc);
    m_rendererInitializeFunc();

    return Result<void>();
}

void ImguiManager::FinalizeImpl() {
    DEV_ASSERT(m_rendererFinalizeFunc);
    m_rendererFinalizeFunc();

    DEV_ASSERT(m_displayFinalizeFunc);
    m_displayFinalizeFunc();

    ImGui::DestroyContext();
}

void ImguiManager::BeginFrame() {
    if (DEV_VERIFY(m_displayBeginFrameFunc)) {
        m_displayBeginFrameFunc();
        ImGui::NewFrame();
    }
}

}  // namespace my
