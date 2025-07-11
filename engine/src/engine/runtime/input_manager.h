#pragma once
#include "engine/core/base/singleton.h"
#include "engine/input/input_router.h"
#include "engine/math/vector.h"
#include "engine/runtime/module.h"

namespace my {

class InputManager : public Singleton<InputManager>,
                     public Module,
                     public MouseButtonBase {
public:
    InputManager() : Module("InputManager") {}

    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    void SetButton(MouseButton p_button, bool p_pressed);

    void BeginFrame();
    void EndFrame();

    void PushInputHandler(IInputHandler* p_input_handler);
    IInputHandler* PopInputHandler();

    // try not to use the following methods, the input should be routered

    Vector2f MouseMove();
    bool IsKeyDown(KeyCode p_key);
    bool IsKeyPressed(KeyCode p_key);
    bool IsKeyReleased(KeyCode p_key);
    Vector2f GetWheel() const;
    const Vector2f& GetCursor() const { return m_cursor; }

protected:
    void SetKey(KeyCode p_key, bool p_pressed);
    void SetCursor(float p_x, float p_y);
    void SetWheel(double p_x, double p_y);

    KeyArray m_keys;
    KeyArray m_prevKeys;

    Vector2f m_cursor{ 0, 0 };
    Vector2f m_prevCursor{ 0, 0 };

    double m_wheelX{ 0 };
    double m_wheelY{ 0 };

    bool m_mouseMoved{ false };

    InputRouter m_router;

    friend class GlfwDisplayManager;
    friend class Win32DisplayManager;
};

};  // namespace my
