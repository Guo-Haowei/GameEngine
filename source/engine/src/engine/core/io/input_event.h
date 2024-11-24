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

class InputEventWithModifier : public IEvent {
public:
    bool IsAltPressed() const { return m_altPressed; }
    bool IsShiftPressed() const { return m_shiftPressed; }
    bool IsCtrlPressed() const { return m_ctrlPressed; }
    bool IsModiferPressed() const { return m_altPressed || m_shiftPressed || m_ctrlPressed; }

protected:
    bool m_altPressed;
    bool m_shiftPressed;
    bool m_ctrlPressed;

    friend class InputManager;
};

class InputEventKey : public InputEventWithModifier {
public:
    bool IsPressed() const { return m_state == InputState::PRESSED; }
    bool IsHolding() const { return m_state == InputState::HOLD; }
    bool IsReleased() const { return m_state == InputState::RELEASED; }

    KeyCode GetKey() const { return m_key; }

protected:
    KeyCode m_key;
    InputState m_state;

    friend class InputManager;
};

class InputEventMouseWheel : public InputEventWithModifier {
public:
    friend class InputManager;

    float GetWheelX() const {
        return m_x;
    }

    float GetWheelY() const {
        return m_y;
    }

protected:
    float m_x;
    float m_y;
};

}  // namespace my
