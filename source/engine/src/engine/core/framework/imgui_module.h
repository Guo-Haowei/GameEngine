#pragma once
#include "core/framework/module.h"

namespace my {

class ImGuiModule : public Module {
public:
    ImGuiModule() : Module("ImGuiModule") {}

    bool Initialize() override;
    void Finalize() override;

protected:
    std::string m_ini_path;
};

}  // namespace my