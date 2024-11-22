#pragma once

namespace my {

enum CommandFlag : uint32_t {
    COMMAND_FLAG_NONE = BIT(0),
    COMMAND_FLAG_REDOABLE = BIT(1),
};
DEFINE_ENUM_BITWISE_OPERATIONS(CommandFlag);

class ICommand {
public:
    ICommand(uint32_t p_type) : m_type(p_type) {}
    virtual ~ICommand() = default;

    // @TODO: rename
    // @TODO: make pure virtual
    virtual void Redo() {}

    uint32_t GetType() const {
        return m_type;
    }

protected:
    uint32_t m_type;
    CommandFlag m_flags;
};

}  // namespace my
