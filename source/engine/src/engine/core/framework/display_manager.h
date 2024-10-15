#pragma once
#include <tuple>

#include "core/base/singleton.h"
#include "core/framework/module.h"

namespace my {

class DisplayManager : public Singleton<DisplayManager>, public Module {
public:
    DisplayManager() : Module("DisplayManager") {}

    virtual bool shouldClose() = 0;

    virtual std::tuple<int, int> getWindowSize() = 0;
    virtual std::tuple<int, int> getWindowPos() = 0;

    virtual void newFrame() = 0;
    virtual void present() = 0;

    static std::shared_ptr<DisplayManager> create();
};

}  // namespace my
