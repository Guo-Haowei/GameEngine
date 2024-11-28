#pragma once
#include "engine/core/framework/module.h"

namespace my {

class ImGuiModule : public Module {
public:
    ImGuiModule() : Module("ImGuiModule") {}

    auto Initialize() -> Result<void> override;
    void Finalize() override;

protected:
    std::string m_iniPath;
};

}  // namespace my