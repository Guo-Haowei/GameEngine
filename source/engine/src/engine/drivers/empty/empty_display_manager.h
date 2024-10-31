#pragma once
#include "core/framework/display_manager.h"

namespace my {

class EmptyDisplayManager : public DisplayManager {
public:
    bool Initialize() override { return true; }
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
};

}  // namespace my
