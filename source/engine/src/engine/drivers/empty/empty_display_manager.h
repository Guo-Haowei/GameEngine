#pragma once
#include "core/framework/display_manager.h"

namespace my {

class EmptyDisplayManager : public DisplayManager {
public:
    bool initialize() override { return true; }
    void finalize() override {}

    bool should_close() override { return true; }

    std::tuple<int, int> get_window_size() override {
        return std::make_tuple(0, 0);
    }
    std::tuple<int, int> get_window_pos() override {
        return std::make_tuple(0, 0);
    }

    void new_frame() override {}
    void present() override {}
};

}  // namespace my
