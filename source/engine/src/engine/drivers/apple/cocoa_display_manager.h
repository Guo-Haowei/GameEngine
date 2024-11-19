#include "core/framework/display_manager.h"
#include "core/input/input_code.h"

namespace my {

class CocoaDisplayManager : public DisplayManager {
public:
    void Finalize() final;

    bool ShouldClose() final;

    std::tuple<int, int> GetWindowSize() final;
    std::tuple<int, int> GetWindowPos() final;

    void NewFrame() final;
    void Present() final;

private:
    bool InitializeWindow() final;
    void InitializeKeyMapping() final;

    // @TODO: fix it
    std::unordered_map<int, KeyCode> m_keyMapping;
};

}  // namespace my
