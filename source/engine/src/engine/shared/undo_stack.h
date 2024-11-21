#pragma once
#include "undo_command.h"

namespace my {

class UndoStack {
public:
    void PushCommand(std::shared_ptr<UndoCommand>&& p_command);

    void Undo();
    void Redo();

    bool CanUndo() const;
    bool CanRedo() const;

private:
    void ClearRedoHistory();

    std::vector<std::shared_ptr<UndoCommand>> m_commands;
    int m_currentCommandIndex{ -1 };
};

}  // namespace my
