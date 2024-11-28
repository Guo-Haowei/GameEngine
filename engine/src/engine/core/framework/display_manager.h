#pragma once
#include <tuple>

#include "engine/core/base/singleton.h"
#include "engine/core/framework/module.h"

namespace my {

enum class KeyCode : uint16_t;

class DisplayManager : public Singleton<DisplayManager>, public Module {
public:
    struct CreateInfo {
        int width;
        int height;
        std::string title;
    };

    DisplayManager() : Module("DisplayManager") {}

    auto Initialize() -> Result<void> final;

    virtual bool ShouldClose() = 0;

    virtual std::tuple<int, int> GetWindowSize() = 0;
    virtual std::tuple<int, int> GetWindowPos() = 0;

    virtual void BeginFrame() = 0;
    virtual void Present() = 0;

    static std::shared_ptr<DisplayManager> Create();

protected:
    virtual auto InitializeWindow(const CreateInfo& p_info) -> Result<void> = 0;
    virtual void InitializeKeyMapping() = 0;

    struct {
        int x, y;
    } m_frameSize, m_windowPos;

    std::unordered_map<int, KeyCode> m_keyMapping;
};

}  // namespace my
