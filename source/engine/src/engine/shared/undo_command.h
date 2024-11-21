#pragma once
#include "command.h"

namespace my {

class UndoCommand : public ICommand {
public:
    UndoCommand(uint32_t p_type) : ICommand(p_type) {}

    virtual void Undo() = 0;
    virtual void Redo() = 0;

    virtual bool MergeCommand(const ICommand*) { return false; }
};

}  // namespace my
