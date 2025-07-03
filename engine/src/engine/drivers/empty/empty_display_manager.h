#pragma once
#include "engine/runtime/display_manager.h"

namespace my {

class EmptyDisplayManager : public DisplayManager {
public:
    void FinalizeImpl() override {}

    bool ShouldClose() override { return true; }

    std::tuple<int, int> GetWindowSize() override {
        return std::make_tuple(0, 0);
    }
    std::tuple<int, int> GetWindowPos() override {
        return std::make_tuple(0, 0);
    }

    void BeginFrame() override {}

protected:
    auto InitializeWindow(const WindowSpecfication&) -> Result<void> override {
        return Result<void>();
    }
    void InitializeKeyMapping() override {};
};

}  // namespace my
