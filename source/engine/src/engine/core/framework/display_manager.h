#pragma once
#include <tuple>

#include "core/base/singleton.h"
#include "core/framework/module.h"

namespace my {

class DisplayManager : public Singleton<DisplayManager>, public Module {
public:
    DisplayManager() : Module("DisplayManager") {}

    virtual bool should_close() = 0;

    virtual std::tuple<int, int> get_window_size() = 0;
    virtual std::tuple<int, int> get_window_pos() = 0;

    virtual void new_frame() = 0;
    virtual void present() = 0;

    static std::shared_ptr<DisplayManager> create();
};

}  // namespace my
