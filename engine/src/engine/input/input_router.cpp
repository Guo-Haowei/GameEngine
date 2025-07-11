#include "input_router.h"

namespace my {

void InputRouter::Route(std::shared_ptr<InputEvent> p_input_event) {
    for (int i = static_cast<int>(m_stack.size()) - 1; i >= 0; --i) {
        const bool handled = m_stack[i]->HandleInput(p_input_event);
        if (handled) {
            break;
        }
    }
}

void InputRouter::PushHandler(IInputHandler* p_handler) {
    DEV_ASSERT(p_handler);
    m_stack.push_back(p_handler);
}

IInputHandler* InputRouter::PopHandler() {
    DEV_ASSERT(!m_stack.empty());
    if (m_stack.empty()) {
        return nullptr;
    }
    IInputHandler* back = m_stack.back();
    m_stack.pop_back();
    return back;
}

}  // namespace my
