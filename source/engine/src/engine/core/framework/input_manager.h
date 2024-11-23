#pragma once
#include <bitset>

#include "core/base/singleton.h"
#include "core/framework/event_queue.h"
#include "core/framework/module.h"
#include "core/io/input_code.h"
#include "core/math/geomath.h"

namespace my {

class InputManager : public Singleton<InputManager>, public Module {
public:
    InputManager() : Module("InputManager") {}

    auto Initialize() -> Result<void> override;
    void Finalize() override;

    void SetButton(int p_button, bool p_pressed);

    void BeginFrame();
    void EndFrame();

    bool IsButtonDown(MouseButton p_key);
    bool IsButtonPressed(MouseButton p_key);

    bool IsKeyDown(KeyCode p_key);
    bool IsKeyPressed(KeyCode p_key);
    bool IsKeyReleased(KeyCode p_key);

    const vec2& GetCursor();
    const vec2& GetWheel();
    vec2 MouseMove();

    void SetKey(KeyCode p_key, bool p_pressed);

    void SetCursor(float p_x, float p_y);
    void SetWheel(float p_x, float p_y);

    EventQueue& GetEventQueue() { return m_inputEventQueue; }

protected:
    using KeyArray = std::bitset<std::to_underlying(KeyCode::COUNT)>;

    EventQueue m_inputEventQueue;

    KeyArray m_keys;
    KeyArray m_prevKeys;

    std::bitset<MOUSE_BUTTON_MAX> m_buttons = { false };
    std::bitset<MOUSE_BUTTON_MAX> m_prevButtons = { false };

    vec2 m_cursor{ 0, 0 };
    vec2 m_prevCursor{ 0, 0 };

    vec2 m_wheel{ 0, 0 };
};

};  // namespace my
