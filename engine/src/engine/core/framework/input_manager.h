#pragma once
#include <bitset>

#include "engine/core/base/singleton.h"
#include "engine/core/framework/event_queue.h"
#include "engine/core/framework/module.h"
#include "engine/core/io/input_code.h"
#include "engine/core/math/geomath.h"

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

    Vector2f MouseMove();

    void SetKey(KeyCode p_key, bool p_pressed);

    void SetCursor(float p_x, float p_y);

    const Vector2f& GetCursor() const { return m_cursor; }

    EventQueue& GetEventQueue() { return m_inputEventQueue; }

    void SetWheel(double p_x, double p_y);

protected:
    using KeyArray = std::bitset<std::to_underlying(KeyCode::COUNT)>;

    EventQueue m_inputEventQueue;

    KeyArray m_keys;
    KeyArray m_prevKeys;

    std::bitset<MOUSE_BUTTON_MAX> m_buttons = { false };
    std::bitset<MOUSE_BUTTON_MAX> m_prevButtons = { false };

    Vector2f m_cursor{ 0, 0 };
    Vector2f m_prevCursor{ 0, 0 };

    double m_wheelX{ 0 };
    double m_wheelY{ 0 };
};

};  // namespace my
