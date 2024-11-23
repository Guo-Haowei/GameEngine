#pragma once
#include "core/framework/event_queue.h"
#include "core/io/input_code.h"

namespace my {

enum class InputState : uint8_t {
    UNKNOWN = 0,
    PRESSED,
    HOLD,
    RELEASED,
    // @TODO: TOGGLE?
};

class InputEventKey : public IEvent {
public:
    bool IsPressed() const { return m_state == InputState::PRESSED; }
    bool IsHolding() const { return m_state == InputState::HOLD; }
    bool IsReleased() const { return m_state == InputState::RELEASED; }

public:
    KeyCode m_key;
    InputState m_state;
    bool m_altPressed;
    bool m_shiftPressed;
    bool m_ctrlPressed;

    friend class InputManager;
};

}  // namespace my
