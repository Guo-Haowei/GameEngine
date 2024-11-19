#pragma once
#include "core/framework/display_manager.h"

namespace my {

class EmptyDisplayManager : public DisplayManager {
public:
    void Finalize() override {}

    bool ShouldClose() override { return true; }

    std::tuple<int, int> GetWindowSize() override {
        return std::make_tuple(0, 0);
    }
    std::tuple<int, int> GetWindowPos() override {
        return std::make_tuple(0, 0);
    }

    void NewFrame() override {}
    void Present() override {}

protected:
    bool InitializeWindow(const CreateInfo&) override { return true; }
    void InitializeKeyMapping() override {};
};

}  // namespace my
