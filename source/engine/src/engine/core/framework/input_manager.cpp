#include "input_manager.h"

namespace my {

auto InputManager::Initialize() -> Result<void> {
    // m_inputEventQueue.RegisterListener();
    return Result<void>();
}

void InputManager::Finalize() {
}

void InputManager::BeginFrame() {
}

void InputManager::EndFrame() {
    m_prevKeys = m_keys;
    m_prevButtons = m_buttons;
    m_prevCursor = m_cursor;
    m_wheel = vec2(0);
}

template<size_t N>
static inline bool InputIsDown(const std::bitset<N>& p_array, int p_index) {
    DEV_ASSERT_INDEX(p_index, N);
    return p_array[p_index];
}

template<size_t N>
static inline bool InputHasChanged(const std::bitset<N>& p_current, const std::bitset<N>& p_prev, int p_index) {
    DEV_ASSERT_INDEX(p_index, N);
    return p_current[p_index] == true && p_prev[p_index] == false;
}

bool InputManager::IsButtonDown(MouseButton p_key) {
    return InputIsDown(m_buttons, std::to_underlying(p_key));
}

bool InputManager::IsButtonPressed(MouseButton p_key) {
    return InputHasChanged(m_buttons, m_prevButtons, std::to_underlying(p_key));
}

bool InputManager::IsKeyDown(KeyCode p_key) {
    return InputIsDown(m_keys, std::to_underlying(p_key));
}

bool InputManager::IsKeyPressed(KeyCode p_key) {
    return InputHasChanged(m_keys, m_prevKeys, std::to_underlying(p_key));
}

bool InputManager::IsKeyReleased(KeyCode p_key) {
    return InputHasChanged(m_prevKeys, m_keys, std::to_underlying(p_key));
}

vec2 InputManager::MouseMove() {
    vec2 point;
    point = m_cursor - m_prevCursor;
    return point;
}

const vec2& InputManager::GetCursor() { return m_cursor; }

const vec2& InputManager::GetWheel() { return m_wheel; }

void InputManager::SetButton(int p_button, bool p_pressed) {
    ERR_FAIL_INDEX(p_button, MOUSE_BUTTON_MAX);
    m_buttons[p_button] = p_pressed;
}

void InputManager::SetKey(KeyCode p_key, bool p_pressed) {
    ERR_FAIL_INDEX(p_key, KeyCode::COUNT);
    const auto index = std::to_underlying(p_key);
    m_keys[index] = p_pressed;

    if (p_pressed && !m_prevKeys[index]) {
        auto e = std::make_shared<KeyPressEvent>();
        e->keys = m_keys;
        e->pressed = static_cast<KeyCode>(p_key);
        m_inputEventQueue.EnqueueEvent(e);
    }
}

void InputManager::SetCursor(float p_x, float p_y) {
    m_cursor.x = p_x;
    m_cursor.y = p_y;
}

void InputManager::SetWheel(float p_x, float p_y) {
    m_wheel.x = p_x;
    m_wheel.y = p_y;
}

}  // namespace my
