#include "undo_stack.h"

namespace my {

void UndoStack::ClearRedoHistory() {

}

void UndoStack::PushCommand(std::shared_ptr<UndoCommand>&& p_command) {
    ClearRedoHistory();

    p_command->Redo();

    if (!m_commands.empty()) {
    }
    
    m_commands.push_back(std::move(p_command));
    m_currentCommandIndex = (int)m_commands.size() - 1;
}

bool UndoStack::CanUndo() const {
    if (m_commands.empty()) {
        return false;
    }

    if (m_currentCommandIndex < 0) {
        return false;
    }

    return true;
}

bool UndoStack::CanRedo() const {
    return false;
}

void UndoStack::Undo() {
    if (!CanUndo()) {
        return;
    }

    auto& command = m_commands[m_currentCommandIndex];

    command->Undo();

    --m_currentCommandIndex;
}

void UndoStack::Redo() {
    if (!CanRedo()) {
        return;
    }

    auto& command = m_commands[m_currentCommandIndex];

    command->Undo();

    ++m_currentCommandIndex;
}

}
