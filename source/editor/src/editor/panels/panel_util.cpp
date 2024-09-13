#include "panel_util.h"

#include "scene/scene.h"

namespace my::panel_util {

void edit_name(NameComponent* name_component) {
    DEV_ASSERT(name_component);
    char buffer[256];
    strncpy(buffer, name_component->getName().c_str(), sizeof(buffer));
    if (ImGui::InputText("##Tag", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        name_component->setName(buffer);
    }
}

}  // namespace my::panel_util
