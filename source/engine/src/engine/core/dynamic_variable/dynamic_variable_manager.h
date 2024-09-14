#pragma once
#include "dynamic_variable.h"

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

    bool parse();

    const std::string& getError() const { return m_error; }

private:
    bool processSetCmd();
    bool processListCmd();

    bool outOfBound();
    const std::string& peek();
    const std::string& consume();

    bool tryGetInt(int& p_out);
    bool tryGetFloat(float& p_out);
    bool tryGetString(std::string_view& p_out);

    const Source m_source;
    const std::vector<std::string>& m_commands;
    size_t m_cursor = 0;
    inline static const std::string s_eof = "<EOF>";
    std::string m_error;
};

class DynamicVariableManager {
public:
    static void serialize();
    static void deserialize();
    static bool parse(const std::vector<std::string>& p_commands);
    static void dumpDvars();
};

}  // namespace my
