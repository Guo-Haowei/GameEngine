#include "dynamic_variable_manager.h"

#include <sstream>

#include "core/io/archive.h"

namespace my {

static constexpr const char* DVAR_CACHE_FILE = "@user://dynamic_variables.cache";

void DynamicVariableManager::serialize() {
    auto res = FileAccess::open(DVAR_CACHE_FILE, FileAccess::WRITE);
    if (!res) {
        LOG_ERROR("{}", res.error().getMessage());
        return;
    }

    LOG("[dvar] serializing dvars");
    auto writer = std::move(*res);

    for (auto const& [key, dvar] : DynamicVariable::s_map) {
        if (dvar->m_flags & DVAR_FLAG_CACHE) {
            auto line = std::format("+set {} {}\n", dvar->m_name, dvar->valueToString());
            writer->writeBuffer(line.data(), line.length());
        }
    }

    writer->close();
}

void DynamicVariableManager::deserialize() {
    auto res = FileAccess::open(DVAR_CACHE_FILE, FileAccess::READ);
    if (!res) {
        LOG_ERROR("{}", res.error().getMessage());
        return;
    }

    auto reader = std::move(*res);
    const size_t size = reader->getLength();
    std::string buffer;
    buffer.resize(size);
    reader->readBuffer(buffer.data(), size);
    reader->close();

    std::stringstream ss{ buffer };
    std::vector<std::string> commands;
    std::string token;
    while (!ss.eof() && ss >> token) {
        commands.emplace_back(token);
    }

    DynamicVariableParser parser(commands, DynamicVariableParser::SOURCE_CACHE);
    if (!parser.parse()) {
        LOG_ERROR("[dvar] Error: {}", parser.getError());
    }
}

void DynamicVariableManager::dumpDvars() {
    for (const auto& it : DynamicVariable::s_map) {
        PRINT("-- {}, '{}'", it.first, it.second->getDesc());
    }
}

//--------------------------------------------------------------------------------------------------
// Dynamic Varialbe Parser
//--------------------------------------------------------------------------------------------------

bool DynamicVariableParser::parse() {
    for (;;) {
        const std::string& command = peek();
        if (command == s_eof) {
            return true;
        }

        if (command == "+set") {
            consume();  // pop set
            if (!processSetCmd()) {
                return false;
            }
        } else if (command == "+list") {
            consume();
            processListCmd();
            return true;
        } else {
            m_error = std::format("unknown command '{}'", command);
            return false;
        }
    }
}

bool DynamicVariableParser::processSetCmd() {
    const std::string& name = consume();
    if (name == s_eof) {
        m_error = "unexpected <EOF>";
        return false;
    }

    DynamicVariable* dvar = DynamicVariable::findDvar(name);
    if (dvar == nullptr) {
        m_error = std::format("dvar '{}' not found", name);
        return false;
    }

    VariantType type = dvar->getType();
    bool ok = true;
    std::string_view str;
    int ix = 0, iy = 0, iz = 0, iw = 0;
    float fx = 0, fy = 0, fz = 0, fw = 0;
    size_t arg_start_index = m_cursor;
    switch (type) {
        case VARIANT_TYPE_INT:
            ok = ok && tryGetInt(ix);
            ok = ok && dvar->setInt(ix);
            break;
        case VARIANT_TYPE_FLOAT:
            ok = ok && tryGetFloat(fx);
            ok = ok && dvar->setFloat(fx);
            break;
        case VARIANT_TYPE_STRING:
            ok = ok && tryGetString(str);
            ok = ok && dvar->setString(str);
            break;
        case VARIANT_TYPE_VEC2:
            ok = ok && tryGetFloat(fx);
            ok = ok && tryGetFloat(fy);
            ok = ok && dvar->setVec2(fx, fy);
            break;
        case VARIANT_TYPE_VEC3:
            ok = ok && tryGetFloat(fx);
            ok = ok && tryGetFloat(fy);
            ok = ok && tryGetFloat(fz);
            ok = ok && dvar->setVec3(fx, fy, fz);
            break;
        case VARIANT_TYPE_VEC4:
            ok = ok && tryGetFloat(fx);
            ok = ok && tryGetFloat(fy);
            ok = ok && tryGetFloat(fz);
            ok = ok && tryGetFloat(fw);
            ok = ok && dvar->setVec4(fx, fy, fz, fw);
            break;
        case VARIANT_TYPE_IVEC2:
            ok = ok && tryGetInt(ix);
            ok = ok && tryGetInt(iy);
            ok = ok && dvar->setIvec2(ix, iy);
            break;
        case VARIANT_TYPE_IVEC3:
            ok = ok && tryGetInt(ix);
            ok = ok && tryGetInt(iy);
            ok = ok && tryGetInt(iz);
            ok = ok && dvar->setIvec3(ix, iy, iz);
            break;
        case VARIANT_TYPE_IVEC4:
            ok = ok && tryGetInt(ix);
            ok = ok && tryGetInt(iy);
            ok = ok && tryGetInt(iz);
            ok = ok && tryGetInt(iw);
            ok = ok && dvar->setIvec4(ix, iy, iz, iw);
            break;
        default:
            CRASH_NOW();
            break;
    }

    if (!ok) {
        m_error = std::format("invalid arguments: +set {}", name);
        for (size_t i = arg_start_index; i < m_commands.size(); ++i) {
            m_error.push_back(' ');
            m_error.append(m_commands[i]);
        }
        return false;
    }

    // @TODO: refactor
    switch (m_source) {
        case SOURCE_CACHE:
            dvar->printValueChange("cache");
            break;
        case SOURCE_COMMAND_LINE:
            dvar->printValueChange("command line");
            break;
        default:
            break;
    }
    return true;
}

bool DynamicVariableParser::processListCmd() {
    DynamicVariableManager::dumpDvars();
    return true;
}

const std::string& DynamicVariableParser::peek() {
    if (outOfBound()) {
        return s_eof;
    }

    return m_commands[m_cursor];
}

const std::string& DynamicVariableParser::consume() {
    if (outOfBound()) {
        return s_eof;
    }

    return m_commands[m_cursor++];
}

bool DynamicVariableParser::tryGetInt(int& p_out) {
    if (outOfBound()) {
        return false;
    }
    const std::string& value = consume();
    if (value == "true") {
        p_out = 1;
    } else if (value == "false") {
        p_out = 0;
    } else {
        p_out = atoi(value.c_str());
    }
    return true;
}

bool DynamicVariableParser::tryGetFloat(float& p_out) {
    if (outOfBound()) {
        return false;
    }
    p_out = (float)atof(consume().c_str());
    return true;
}

bool DynamicVariableParser::tryGetString(std::string_view& p_out) {
    if (outOfBound()) {
        return false;
    }

    const std::string& next = consume();
    if (next.length() >= 2 && next.front() == '"' && next.back() == '"') {
        p_out = std::string_view(next.data() + 1, next.length() - 2);
    } else {
        p_out = std::string_view(next.data(), next.length());
    }

    return true;
}

bool DynamicVariableParser::outOfBound() {
    return m_cursor >= m_commands.size();
}

bool DynamicVariableManager::parse(const std::vector<std::string>& p_commands) {
    DynamicVariableParser parser(p_commands, DynamicVariableParser::SOURCE_COMMAND_LINE);
    bool ok = parser.parse();
    if (!ok) {
        LOG_ERROR("[dvar] Error: {}", parser.getError());
    }
    return ok;
}

}  // namespace my