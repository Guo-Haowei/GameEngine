#include "panel_util.h"

#include "engine/scene/scene.h"

namespace my::panel_util {

void InputText(std::string& p_string, const char* p_tag) {
    char buffer[256];
    strncpy(buffer, p_string.c_str(), sizeof(buffer));
    if (ImGui::InputText(p_tag, buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        p_string = buffer;
    }
}

}  // namespace my::panel_util
