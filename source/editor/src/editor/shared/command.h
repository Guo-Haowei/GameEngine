#pragma once

namespace my {

enum class CommandType : uint8_t {
    TRANSLATE_ENTITY,
    ROTATE_ENTITY,
    SCALE_ENTITY,

    ADD_ENTITY,
    REMOVE_ENTITY,
    ADD_COMPONENT,
    COUNT,
};

enum CommandFlag : uint32_t {
    COMMAND_FLAG_NONE = BIT(0),
    COMMAND_FLAG_REDOABLE = BIT(1),
};
DEFINE_ENUM_BITWISE_OPERATIONS(CommandFlag);

class ICommand {
public:
    ICommand(CommandType p_type) : type(p_type) {}
    virtual ~ICommand() = default;

protected:
    CommandType type;
    CommandFlag flags;
};

}
