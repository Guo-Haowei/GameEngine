#pragma once
#include "dynamic_variable.h"

#if USING(ENABLE_DVAR)
namespace my {

class DynamicVariableParser {
public:
    enum Source {
        SOURCE_NONE,
        SOURCE_COMMAND_LINE,
        SOURCE_CACHE,
    };

    DynamicVariableParser(const std::vector<std::string>& p_commands, Source p_source)
        : m_source(p_source), m_commands(p_commands) {}

    bool Parse();

    const std::string& GetError() const { return m_error; }

private:
    bool ProcessSetCmd();
    bool ProcessListCmd();

    bool OutOfBound();
    const std::string& Peek();
    const std::string& Consume();

    bool TryGetInt(int& p_out);
    bool TryGetFloat(float& p_out);
    bool TryGetString(std::string_view& p_out);

    const Source m_source;
    const std::vector<std::string>& m_commands;
    size_t m_cursor = 0;
    inline static const std::string s_eof = "<EOF>";
    std::string m_error;
};

class DynamicVariableManager {
public:
    static void Serialize(std::string_view p_path);
    static void Deserialize(std::string_view p_path);
    static bool Parse(const std::vector<std::string>& p_commands);
    static void DumpDvars();
};

}  // namespace my
#endif
