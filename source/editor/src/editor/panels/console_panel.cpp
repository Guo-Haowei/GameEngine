#include "console_panel.h"

#include "core/io/logger.h"
#include "core/math/color.h"

namespace my {

static ImVec4 log_level_to_color(LogLevel level) {
    Color color = Color::hex(ColorCode::COLOR_WHITE);
    switch (level) {
        case my::LOG_LEVEL_VERBOSE:
            color = Color::hex(ColorCode::COLOR_SILVER);
            break;
        case my::LOG_LEVEL_OK:
            color = Color::hex(ColorCode::COLOR_GREEN);
            break;
        case my::LOG_LEVEL_WARN:
            color = Color::hex(ColorCode::COLOR_YELLOW);
            break;
        case my::LOG_LEVEL_ERROR:
        case my::LOG_LEVEL_FATAL:
            color = Color::hex(ColorCode::COLOR_RED);
            break;
        default:
            break;
    }

    return ImVec4(color.r, color.g, color.b, 1.0f);
}

void ConsolePanel::update_internal(Scene&) {
    ImGui::Separator();

    // reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));  // Tighten spacing

    std::vector<my::CompositeLogger::Log> logs;
    my::CompositeLogger::singleton().retrieve_log(logs);
    for (const auto& log : logs) {
        ImGui::PushStyleColor(ImGuiCol_Text, log_level_to_color(log.level));
        ImGui::TextUnformatted(log.buffer);
        ImGui::PopStyleColor();
    }

    if (m_scroll_to_bottom || (m_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
        ImGui::SetScrollHereY(1.0f);
    }
    m_scroll_to_bottom = false;

    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();

    ImGui::SameLine();

    ImGui::Separator();

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
}

}  // namespace my
