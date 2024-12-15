#include "dynamic_variable_manager.h"

#include <sstream>

#include "engine/core/io/archive.h"
#include "engine/core/string/string_builder.h"

namespace my {

void DynamicVariableManager::Serialize(std::string_view p_path) {
    auto res = FileAccess::Open(p_path, FileAccess::WRITE);
    if (!res) {
        StringStreamBuilder builder;
        builder << res.error();
        LOG_ERROR("{}", builder.ToString());
        return;
    }

    LOG("[dvar] serializing dvars");
    auto writer = std::move(*res);

    for (auto const& [key, dvar] : DynamicVariable::s_map) {
        if (dvar->m_flags & DVAR_FLAG_CACHE) {
            auto line = std::format("+set {} {}\n", dvar->m_name, dvar->ValueToString());
            writer->WriteBuffer(line.data(), line.length());
        }
    }

    writer->Close();
}

void DynamicVariableManager::Deserialize(std::string_view p_path) {
    auto res = FileAccess::Open(p_path, FileAccess::READ);
    if (!res) {
        if (res.error()->value != ErrorCode::ERR_FILE_NOT_FOUND) {
            StringStreamBuilder builder;
            builder << res.error();
            LOG_ERROR("{}", builder.ToString());
        }
        return;
    }

    auto reader = std::move(*res);
    const size_t size = reader->GetLength();
    std::string buffer;
    buffer.resize(size);
    reader->ReadBuffer(buffer.data(), size);
    reader->Close();

    std::stringstream ss{ buffer };
    std::vector<std::string> commands;
    std::string token;
    while (!ss.eof() && ss >> token) {
        commands.emplace_back(token);
    }

    DynamicVariableParser parser(commands, DynamicVariableParser::SOURCE_CACHE);
    if (!parser.Parse()) {
        LOG_ERROR("[dvar] Error: {}", parser.GetError());
    }
}

void DynamicVariableManager::DumpDvars() {
    for (const auto& it : DynamicVariable::s_map) {
        PRINT("-- {}, '{}'", it.first, it.second->GetDesc());
    }
}

//--------------------------------------------------------------------------------------------------
// Dynamic Varialbe Parser
//--------------------------------------------------------------------------------------------------

bool DynamicVariableParser::Parse() {
    for (;;) {
        const std::string& command = Peek();
        if (command == s_eof) {
            return true;
        }

        if (command == "+set") {
            Consume();  // pop set
            if (!ProcessSetCmd()) {
                return false;
            }
        } else if (command == "+list") {
            Consume();
            ProcessListCmd();
            return true;
        } else {
            m_error = std::format("unknown command '{}'", command);
            return false;
        }
    }
}

bool DynamicVariableParser::ProcessSetCmd() {
    const std::string& name = Consume();
    if (name == s_eof) {
        m_error = "unexpected <EOF>";
        return false;
    }

    DynamicVariable* dvar = DynamicVariable::FindDvar(name);
    if (dvar == nullptr) {
        m_error = std::format("dvar '{}' not found", name);
        return false;
    }

    VariantType type = dvar->GetType();
    bool ok = true;
    std::string_view str;
    int ix = 0, iy = 0, iz = 0, iw = 0;
    float fx = 0, fy = 0, fz = 0, fw = 0;
    size_t arg_start_index = m_cursor;
    const bool overriden = dvar->GetFlags() & DVAR_FLAG_OVERRIDEN;
    switch (type) {
        case VARIANT_TYPE_INT: {
            ok = ok && TryGetInt(ix);
            if (!overriden) {
                ok = ok && dvar->SetInt(ix);
            }
        } break;
        case VARIANT_TYPE_FLOAT: {
            ok = ok && TryGetFloat(fx);
            if (!overriden) {
                ok = ok && dvar->SetFloat(fx);
            }
        } break;
        case VARIANT_TYPE_STRING: {
            ok = ok && TryGetString(str);
            if (!overriden) {
                ok = ok && dvar->SetString(str);
            }
        } break;
        case VARIANT_TYPE_VEC2: {
            ok = ok && TryGetFloat(fx);
            ok = ok && TryGetFloat(fy);
            if (!overriden) {
                ok = ok && dvar->SetVector2f(fx, fy);
            }
        } break;
        case VARIANT_TYPE_VEC3: {
            ok = ok && TryGetFloat(fx);
            ok = ok && TryGetFloat(fy);
            ok = ok && TryGetFloat(fz);
            if (!overriden) {
                ok = ok && dvar->SetVector3f(fx, fy, fz);
            }
        } break;
        case VARIANT_TYPE_VEC4: {
            ok = ok && TryGetFloat(fx);
            ok = ok && TryGetFloat(fy);
            ok = ok && TryGetFloat(fz);
            ok = ok && TryGetFloat(fw);
            if (!overriden) {
                ok = ok && dvar->SetVector4f(fx, fy, fz, fw);
            }
        } break;
        case VARIANT_TYPE_IVEC2: {
            ok = ok && TryGetInt(ix);
            ok = ok && TryGetInt(iy);
            if (!overriden) {
                ok = ok && dvar->SetVector2i(ix, iy);
            }
        } break;
        case VARIANT_TYPE_IVEC3: {
            ok = ok && TryGetInt(ix);
            ok = ok && TryGetInt(iy);
            ok = ok && TryGetInt(iz);
            if (!overriden) {
                ok = ok && dvar->SetVector3i(ix, iy, iz);
            }
        } break;
        case VARIANT_TYPE_IVEC4: {
            ok = ok && TryGetInt(ix);
            ok = ok && TryGetInt(iy);
            ok = ok && TryGetInt(iz);
            ok = ok && TryGetInt(iw);
            if (!overriden) {
                ok = ok && dvar->SetVector4i(ix, iy, iz, iw);
            }
        } break;
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
            dvar->PrintValueChange("cache");
            break;
        case SOURCE_COMMAND_LINE:
            dvar->PrintValueChange("command line");
            break;
        default:
            break;
    }
    return true;
}

bool DynamicVariableParser::ProcessListCmd() {
    DynamicVariableManager::DumpDvars();
    return true;
}

const std::string& DynamicVariableParser::Peek() {
    if (OutOfBound()) {
        return s_eof;
    }

    return m_commands[m_cursor];
}

const std::string& DynamicVariableParser::Consume() {
    if (OutOfBound()) {
        return s_eof;
    }

    return m_commands[m_cursor++];
}

bool DynamicVariableParser::TryGetInt(int& p_out) {
    if (OutOfBound()) {
        return false;
    }
    const std::string& value = Consume();
    if (value == "true") {
        p_out = 1;
    } else if (value == "false") {
        p_out = 0;
    } else {
        p_out = atoi(value.c_str());
    }
    return true;
}

bool DynamicVariableParser::TryGetFloat(float& p_out) {
    if (OutOfBound()) {
        return false;
    }
    p_out = (float)atof(Consume().c_str());
    return true;
}

bool DynamicVariableParser::TryGetString(std::string_view& p_out) {
    if (OutOfBound()) {
        return false;
    }

    const std::string& next = Consume();
    if (next.length() >= 2 && next.front() == '"' && next.back() == '"') {
        p_out = std::string_view(next.data() + 1, next.length() - 2);
    } else {
        p_out = std::string_view(next.data(), next.length());
    }

    return true;
}

bool DynamicVariableParser::OutOfBound() {
    return m_cursor >= m_commands.size();
}

bool DynamicVariableManager::Parse(const std::vector<std::string>& p_commands) {
    DynamicVariableParser parser(p_commands, DynamicVariableParser::SOURCE_COMMAND_LINE);
    bool ok = parser.Parse();
    if (!ok) {
        LOG_ERROR("[dvar] Error: {}", parser.GetError());
    }
    return ok;
}

}  // namespace my