
#pragma once
#include "engine/core/input/input_event.h"

namespace my {

class IInputHandler {
public:
    virtual bool HandleInput(std::shared_ptr<InputEvent> p_input_event) = 0;
};

class InputRouter {
public:
    void Route(std::shared_ptr<InputEvent> p_input_event);

    void PushHandler(IInputHandler* p_handler);

    IInputHandler* PopHandler();

private:
    std::vector<IInputHandler*> m_stack;
};

}  // namespace my
