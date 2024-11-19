#include "core/framework/display_manager.h"
#include "core/input/input_code.h"

namespace my {

class NSWindowWrapperImpl;

class CocoaDisplayManager : public DisplayManager {
public:
    CocoaDisplayManager();

    void Finalize() final;

    bool ShouldClose() final;

    std::tuple<int, int> GetWindowSize() final;
    std::tuple<int, int> GetWindowPos() final;

    void NewFrame() final;
    void Present() final;

private:
    bool InitializeWindow() final;
    void InitializeKeyMapping() final;

    std::shared_ptr<NSWindowWrapperImpl> m_impl;
};

}  // namespace my
