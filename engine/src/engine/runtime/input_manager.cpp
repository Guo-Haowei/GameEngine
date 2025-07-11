#include "input_manager.h"

#include "engine/core/input/input_event.h"

namespace my {

auto InputManager::InitializeImpl() -> Result<void> {
    return Result<void>();
}

void InputManager::FinalizeImpl() {
}

void InputManager::BeginFrame() {
    const bool alt = IsKeyDown(KeyCode::KEY_LEFT_ALT) || IsKeyDown(KeyCode::KEY_RIGHT_ALT);
    const bool ctrl = IsKeyDown(KeyCode::KEY_LEFT_CONTROL) || IsKeyDown(KeyCode::KEY_RIGHT_CONTROL);
    const bool shift = IsKeyDown(KeyCode::KEY_LEFT_SHIFT) || IsKeyDown(KeyCode::KEY_RIGHT_SHIFT);
    const bool modifier_pressed = alt || ctrl || shift;

    // Send key events
    for (int i = 0; i < std::to_underlying(KeyCode::COUNT); ++i) {
        const auto value = m_keys[i];
        const auto prev_value = m_prevKeys[i];

        auto get_state = [&]() {
            if (value == true && prev_value == false) {
                return InputState::PRESSED;
            }
            if (value == false && prev_value != true) {
                return InputState::RELEASED;
            }
            if (value == true && !modifier_pressed) {
                return InputState::HOLD;
            }
            return InputState::UNKNOWN;
        };

        InputState state = get_state();
        if (state == InputState::UNKNOWN) {
            continue;
        }
        auto e = std::make_shared<InputEventKey>();
        e->m_key = static_cast<KeyCode>(i);
        e->m_state = state;
        e->m_altPressed = alt;
        e->m_ctrlPressed = ctrl;
        e->m_shiftPressed = shift;

        m_router.Route(e);
    }

    // Send mouse wheel events
    if (m_wheelX != 0 || m_wheelY != 0) {
        auto e = std::make_shared<InputEventMouseWheel>(m_buttons,
                                                        m_prevButtons,
                                                        Vector2f(static_cast<float>(m_wheelX), static_cast<float>(m_wheelY)));
        e->m_altPressed = alt;
        e->m_ctrlPressed = ctrl;
        e->m_shiftPressed = shift;

        m_router.Route(e);
    }

    // Send mouse moved event
    if (m_mouseMoved) {
        auto e = std::make_shared<InputEventMouseMove>(m_buttons, m_prevButtons);
        e->m_pos = m_cursor;
        e->m_prevPos = m_prevCursor;
        e->m_altPressed = alt;
        e->m_ctrlPressed = ctrl;
        e->m_shiftPressed = shift;

        m_router.Route(e);
    }
}

void InputManager::EndFrame() {
    m_prevKeys = m_keys;
    m_prevButtons = m_buttons;
    m_prevCursor = m_cursor;

    m_wheelX = 0;
    m_wheelY = 0;

    m_mouseMoved = false;
}

void InputManager::PushInputHandler(IInputHandler* p_input_handler) {
    m_router.PushHandler(p_input_handler);
}

IInputHandler* InputManager::PopInputHandler() {
    return m_router.PopHandler();
}

bool InputManager::IsKeyDown(KeyCode p_key) {
    return m_keys[std::to_underlying(p_key)];
}

bool InputManager::IsKeyPressed(KeyCode p_key) {
    unused(p_key);
    CRASH_NOW();
    return false;
    // return InputHasChanged(m_keys, m_prevKeys, std::to_underlying(p_key));
}

bool InputManager::IsKeyReleased(KeyCode p_key) {
    unused(p_key);
    CRASH_NOW();
    return false;
    // return InputHasChanged(m_prevKeys, m_keys, std::to_underlying(p_key));
}

Vector2f InputManager::MouseMove() {
    Vector2f point;
    point = m_cursor - m_prevCursor;
    return point;
}

void InputManager::SetButton(MouseButton p_button, bool p_pressed) {
    ERR_FAIL_INDEX(p_button, MouseButton::COUNT);
    m_buttons[std::to_underlying(p_button)] = p_pressed;
}

void InputManager::SetKey(KeyCode p_key, bool p_pressed) {
    ERR_FAIL_INDEX(p_key, KeyCode::COUNT);
    const auto index = std::to_underlying(p_key);
    m_keys[index] = p_pressed;
}

void InputManager::SetCursor(float p_x, float p_y) {
    m_cursor.x = p_x;
    m_cursor.y = p_y;

    m_mouseMoved = true;
}

void InputManager::SetWheel(double p_x, double p_y) {
    m_wheelX = p_x;
    m_wheelY = p_y;
}

Vector2f InputManager::GetWheel() const {
    return Vector2f(m_wheelX, m_wheelY);
}

}  // namespace my
