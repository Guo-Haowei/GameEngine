#pragma once

namespace my {

class UndoCommand {
public:
    UndoCommand() {}

    virtual void Undo() = 0;
    virtual void Redo() = 0;

    virtual bool MergeCommand(const UndoCommand*) { return false; }
};

}  // namespace my
