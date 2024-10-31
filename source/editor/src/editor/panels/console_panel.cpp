#include "console_panel.h"

#include "core/io/logger.h"
#include "core/math/color.h"

namespace my {

static ImVec4 GetLogLevelColor(LogLevel level) {
    Color color = Color::Hex(ColorCode::COLOR_WHITE);
    switch (level) {
        case my::LOG_LEVEL_VERBOSE:
            color = Color::Hex(ColorCode::COLOR_SILVER);
            break;
        case my::LOG_LEVEL_OK:
            color = Color::Hex(ColorCode::COLOR_GREEN);
            break;
        case my::LOG_LEVEL_WARN:
            color = Color::Hex(ColorCode::COLOR_YELLOW);
            break;
        case my::LOG_LEVEL_ERROR:
        case my::LOG_LEVEL_FATAL:
            color = Color::Hex(ColorCode::COLOR_RED);
            break;
        default:
            break;
    }

    return ImVec4(color.r, color.g, color.b, 1.0f);
}

void ConsolePanel::UpdateInternal(Scene&) {
    ImGui::Separator();

    // reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));  // Tighten spacing

    std::vector<my::CompositeLogger::Log> logs;
    my::CompositeLogger::GetSingleton().RetrieveLog(logs);
    for (const auto& log : logs) {
        ImGui::PushStyleColor(ImGuiCol_Text, GetLogLevelColor(log.level));
        ImGui::TextUnformatted(log.buffer);
        ImGui::PopStyleColor();
    }

    if (m_scrollToBottom || (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
        ImGui::SetScrollHereY(1.0f);
    }
    m_scrollToBottom = false;

    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();

    ImGui::SameLine();

    ImGui::Separator();

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
}

}  // namespace my
