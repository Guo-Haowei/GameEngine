#include "engine/systems/undo_redo/undo_stack.h"

namespace my {

class IncrementCommand : public UndoCommand {
public:
    IncrementCommand(int& p_ref, std::string& p_message) : m_counter(p_ref), m_message(p_message) {
    }

    void Undo() override {
        const int prev = m_counter--;
        m_message += std::format("Undo:{}->{};", prev, m_counter);
    }

    void Redo() override {
        const int prev = m_counter++;
        m_message += std::format("Redo:{}->{};", prev, m_counter);
    }

    int GetCounter() const { return m_counter; }

private:
    int& m_counter;
    std::string& m_message;
};
TEST(undo_stack, push_command_calls_redo) {
    std::string message;
    int counter = 0;
    UndoStack stack;

    stack.PushCommand(std::make_shared<IncrementCommand>(counter, message));
    EXPECT_EQ(message, "Redo:0->1;");
    stack.PushCommand(std::make_shared<IncrementCommand>(counter, message));
    stack.PushCommand(std::make_shared<IncrementCommand>(counter, message));
    EXPECT_EQ(message, "Redo:0->1;Redo:1->2;Redo:2->3;");
}

TEST(undo_stack, can_undo) {
    std::string message;
    int counter = 0;
    UndoStack stack;

    EXPECT_FALSE(stack.CanUndo());
    stack.PushCommand(std::make_shared<IncrementCommand>(counter, message));
    EXPECT_TRUE(stack.CanUndo());
    stack.Undo();
    EXPECT_FALSE(stack.CanUndo());
    stack.Redo();
    EXPECT_TRUE(stack.CanUndo());
}

TEST(undo_stack, can_redo) {
    std::string message;
    int counter = 0;
    UndoStack stack;

    EXPECT_FALSE(stack.CanRedo());
    stack.PushCommand(std::make_shared<IncrementCommand>(counter, message));
    EXPECT_FALSE(stack.CanRedo());
    stack.Undo();
    EXPECT_TRUE(stack.CanRedo());
    stack.Redo();
    EXPECT_FALSE(stack.CanRedo());
}

TEST(undo_stack, push_command_clear_redo_history) {
    std::string message;
    int counter = 0;
    UndoStack stack;

    stack.PushCommand(std::make_shared<IncrementCommand>(counter, message));
    EXPECT_EQ(message, "Redo:0->1;");
    stack.PushCommand(std::make_shared<IncrementCommand>(counter, message));
    stack.PushCommand(std::make_shared<IncrementCommand>(counter, message));
    EXPECT_EQ(message, "Redo:0->1;Redo:1->2;Redo:2->3;");

    message.clear();

    stack.Undo();
    EXPECT_EQ(message, "Undo:3->2;");
    stack.Undo();
    EXPECT_EQ(message, "Undo:3->2;Undo:2->1;");
    stack.PushCommand(std::make_shared<IncrementCommand>(counter, message));
    EXPECT_FALSE(stack.CanRedo());
    EXPECT_TRUE(stack.CanUndo());
    stack.Undo();
    stack.Redo();
    EXPECT_EQ(message, "Undo:3->2;Undo:2->1;Redo:1->2;Undo:2->1;Redo:1->2;");
}

}  // namespace my
