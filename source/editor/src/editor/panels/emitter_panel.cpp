#include "emitter_panel.h"

namespace my {

void EmitterPanel::UpdateInternal(Scene&) {
    // ImGui::Separator();

    // // reserve enough left-over height for 1 separator + 1 input text
    // const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    // ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
    //                   ImGuiWindowFlags_HorizontalScrollbar);

    // ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));  // Tighten spacing

    // std::vector<my::CompositeLogger::Log> logs;
    // my::CompositeLogger::singleton().RetrieveLog(logs);
    // for (const auto& log : logs) {
    //     ImGui::PushStyleColor(ImGuiCol_Text, log_level_to_color(log.level));
    //     ImGui::TextUnformatted(log.buffer);
    //     ImGui::PopStyleColor();
    // }

    // if (m_scroll_to_bottom || (m_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
    //     ImGui::SetScrollHereY(1.0f);
    // }
    // m_scroll_to_bottom = false;

    // ImGui::PopStyleVar();
    // ImGui::EndChild();
    // ImGui::Separator();

    // ImGui::SameLine();

    // ImGui::Separator();

    // // Auto-focus on window apparition
    // ImGui::SetItemDefaultFocus();
}

}  // namespace my
