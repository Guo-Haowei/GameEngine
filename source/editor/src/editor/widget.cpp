#include "widget.h"

namespace my {

void PushDisabled() {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
}

void PopDisabled() {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
}

bool DrawDragFloat(const char* p_lable,
                   float& p_out_float,
                   float p_speed,
                   float p_min,
                   float p_max,
                   float p_column_width) {
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, p_column_width);
    ImGui::Text(p_lable);
    ImGui::NextColumn();
    auto dragFloatTag = std::format("##{}", p_lable);
    bool dirty = ImGui::DragFloat(dragFloatTag.c_str(), &p_out_float, p_speed, p_min, p_max);
    ImGui::Columns(1);
    return dirty;
}
enum {
    TYPE_TRANSFORM,
    TYPE_COLOR,
};

static bool DrawVec3ControlImpl(int type,
                                const char* p_label,
                                glm::vec3& p_out_vec3,
                                float p_reset_value,
                                float p_column_width) {
    bool dirty = false;

    ImGuiIO& io = ImGui::GetIO();
    auto bold_font = io.Fonts->Fonts[0];

    const char* button_names[3];
    float speed = 0.1f;
    float min = 0.0f;
    float max = 0.0f;
    if (type == TYPE_COLOR) {
        button_names[0] = "R";
        button_names[1] = "G";
        button_names[2] = "B";
        speed = 0.01f;
        min = 0.0f;
        max = 1.0f;
    } else {
        button_names[0] = "X";
        button_names[1] = "Y";
        button_names[2] = "Z";
    }

    ImGui::PushID(p_label);

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, p_column_width);
    ImGui::Text(p_label);
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushFont(bold_font);

    auto draw_button = [&](int idx) {
        if (ImGui::Button(button_names[idx])) {
            p_out_vec3[idx] = p_reset_value;
            dirty = true;
        }
    };

    draw_button(0);

    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    dirty |= ImGui::DragFloat("##X", &p_out_vec3.x, speed, min, max, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushFont(bold_font);

    draw_button(1);

    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    dirty |= ImGui::DragFloat("##Y", &p_out_vec3.y, speed, min, max, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushFont(bold_font);
    draw_button(2);
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    dirty |= ImGui::DragFloat("##Z", &p_out_vec3.z, speed, min, max, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();

    ImGui::Columns(1);

    ImGui::PopID();
    return dirty;
}

bool DrawVec3Control(const char* p_lable,
                     glm::vec3& p_out_vec3,
                     float p_reset_value,
                     float p_column_width) {
    return DrawVec3ControlImpl(TYPE_TRANSFORM, p_lable, p_out_vec3, p_reset_value, p_column_width);
}

bool DrawColorControl(const char* p_lable,
                      glm::vec3& p_out_vec3,
                      float p_reset_value,
                      float p_column_width) {
    return DrawVec3ControlImpl(TYPE_COLOR, p_lable, p_out_vec3, p_reset_value, p_column_width);
}

}  // namespace my
