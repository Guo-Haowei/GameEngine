#pragma once
#include "core/framework/display_manager.h"

namespace my {

class EmptyDisplayManager : public DisplayManager {
public:
    bool Initialize() override { return true; }
    void Finalize() override {}

    bool shouldClose() override { return true; }

    std::tuple<int, int> getWindowSize() override {
        return std::make_tuple(0, 0);
    }
    std::tuple<int, int> getWindowPos() override {
        return std::make_tuple(0, 0);
    }

    void newFrame() override {}
    void present() override {}
};

}  // namespace my
