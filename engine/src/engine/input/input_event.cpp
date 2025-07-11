#include "engine/input/input_event.h"

namespace my {

bool MouseButtonBase::IsButtonDown(MouseButton p_button) const {
    return InputIsDown(m_buttons, std::to_underlying(p_button));
}

bool MouseButtonBase::IsButtonUp(MouseButton p_button) const {
    return !InputIsDown(m_buttons, std::to_underlying(p_button));
}

bool MouseButtonBase::IsButtonPressed(MouseButton p_button) const {
    return InputHasChanged(m_buttons, m_prevButtons, std::to_underlying(p_button));
}

bool MouseButtonBase::IsButtonReleased(MouseButton p_button) const {
    return InputHasChanged(m_prevButtons, m_buttons, std::to_underlying(p_button));
}

}  // namespace my
