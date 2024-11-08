#pragma once
#include <tuple>

#include "core/base/singleton.h"
#include "core/framework/module.h"

namespace my {

class DisplayManager : public Singleton<DisplayManager>, public Module {
public:
    DisplayManager() : Module("DisplayManager") {}

    bool Initialize() final;

    virtual bool ShouldClose() = 0;

    virtual std::tuple<int, int> GetWindowSize() = 0;
    virtual std::tuple<int, int> GetWindowPos() = 0;

    virtual void NewFrame() = 0;
    virtual void Present() = 0;

    static std::shared_ptr<DisplayManager> Create();

protected:
    virtual bool InitializeWindow() = 0;
    virtual void InitializeKeyMapping() = 0;

    struct {
        int x, y;
    } m_frameSize, m_windowPos;
};

}  // namespace my
