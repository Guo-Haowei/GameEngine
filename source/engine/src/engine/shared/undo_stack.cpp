#include "undo_stack.h"

namespace my {

void UndoStack::ClearRedoHistory() {
    if (DEV_VERIFY(m_currentCommandIndex < (int)m_commands.size())) {
        m_commands.resize(m_currentCommandIndex + 1);
    }
}

void UndoStack::PushCommand(std::shared_ptr<UndoCommand>&& p_command) {
    ClearRedoHistory();

    p_command->Redo();

    if (!m_commands.empty()) {
        if (m_commands.back()->MergeCommand(p_command.get())) {
            return;
        }
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
    if (m_commands.empty()) {
        return false;
    }

    // there's one command after current one
    // so the difference of index and size should be at least 2
    if ((int)m_commands.size() - m_currentCommandIndex < 2) {
        return false;
    }

    return true;
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

    auto& command = m_commands[m_currentCommandIndex + 1];

    command->Redo();

    ++m_currentCommandIndex;
}

}  // namespace my
