#include "dynamic_variable.h"

#if USING(ENABLE_DVAR)

namespace my {

DynamicVariable::DynamicVariable(VariantType p_type, DvarFlags p_flags, const char* p_desc)
    : m_type(p_type), m_desc(p_desc), m_flags(p_flags), m_int(0) {
}

void DynamicVariable::RegisterInt(std::string_view p_key, int p_value) {
    if (!(m_flags & DVAR_FLAG_OVERRIDEN)) {
        SetInt(p_value);
    }
    RegisterDvar(p_key, this);
}

void DynamicVariable::RegisterFloat(std::string_view p_key, float p_value) {
    if (!(m_flags & DVAR_FLAG_OVERRIDEN)) {
        SetFloat(p_value);
    }
    RegisterDvar(p_key, this);
}

void DynamicVariable::RegisterString(std::string_view p_key, std::string_view p_value) {
    if (!(m_flags & DVAR_FLAG_OVERRIDEN)) {
        SetString(p_value);
    }
    RegisterDvar(p_key, this);
}

void DynamicVariable::RegisterVector2f(std::string_view p_key, float p_x, float p_y) {
    if (!(m_flags & DVAR_FLAG_OVERRIDEN)) {
        m_vec.x = p_x;
        m_vec.y = p_y;
    }
    RegisterDvar(p_key, this);
}

void DynamicVariable::RegisterVector3f(std::string_view p_key, float p_x, float p_y, float p_z) {
    if (!(m_flags & DVAR_FLAG_OVERRIDEN)) {
        m_vec.x = p_x;
        m_vec.y = p_y;
        m_vec.z = p_z;
    }
    RegisterDvar(p_key, this);
}

void DynamicVariable::RegisterVector4f(std::string_view p_key, float p_x, float p_y, float p_z, float p_w) {
    if (!(m_flags & DVAR_FLAG_OVERRIDEN)) {
        m_vec.x = p_x;
        m_vec.y = p_y;
        m_vec.z = p_z;
        m_vec.w = p_w;
    }
    RegisterDvar(p_key, this);
}

void DynamicVariable::RegisterVector2i(std::string_view p_key, int p_x, int p_y) {
    if (!(m_flags & DVAR_FLAG_OVERRIDEN)) {
        m_ivec.x = p_x;
        m_ivec.y = p_y;
    }
    RegisterDvar(p_key, this);
}

void DynamicVariable::RegisterVector3i(std::string_view p_key, int p_x, int p_y, int p_z) {
    if (!(m_flags & DVAR_FLAG_OVERRIDEN)) {
        m_ivec.x = p_x;
        m_ivec.y = p_y;
        m_ivec.z = p_z;
    }
    RegisterDvar(p_key, this);
}

void DynamicVariable::RegisterVector4i(std::string_view p_key, int p_x, int p_y, int p_z, int p_w) {
    if (!(m_flags & DVAR_FLAG_OVERRIDEN)) {
        m_ivec.x = p_x;
        m_ivec.y = p_y;
        m_ivec.z = p_z;
        m_ivec.w = p_w;
    }
    RegisterDvar(p_key, this);
}

int DynamicVariable::AsInt() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_INT);
    return m_int;
}

float DynamicVariable::AsFloat() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_FLOAT);
    return m_float;
}

const std::string& DynamicVariable::AsString() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_STRING);
    return m_string;
}

Vector2f DynamicVariable::AsVector2f() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_VEC2);
    return Vector2f(m_vec.x, m_vec.y);
}

Vector3f DynamicVariable::AsVector3f() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_VEC3);
    return Vector3f(m_vec.x, m_vec.y, m_vec.z);
}

Vector4f DynamicVariable::AsVector4f() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_VEC4);
    return m_vec;
}

Vector2i DynamicVariable::AsVector2i() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_IVEC2);
    return Vector2i(m_ivec.x, m_ivec.y);
}

Vector3i DynamicVariable::AsVector3i() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_IVEC3);
    return Vector3i(m_ivec.x, m_ivec.y, m_ivec.z);
}

Vector4i DynamicVariable::AsVector4i() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_IVEC4);
    return m_ivec;
}

void* DynamicVariable::AsPointer() {
    switch (m_type) {
        case VARIANT_TYPE_INT:
        case VARIANT_TYPE_FLOAT:
        case VARIANT_TYPE_VEC2:
        case VARIANT_TYPE_VEC3:
        case VARIANT_TYPE_VEC4:
        case VARIANT_TYPE_IVEC2:
        case VARIANT_TYPE_IVEC3:
        case VARIANT_TYPE_IVEC4:
            return &m_int;
        default:
            CRASH_NOW();
            return nullptr;
    }
}

bool DynamicVariable::SetInt(int p_value) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_INT, false);
    m_int = p_value;
    return true;
}

bool DynamicVariable::SetFloat(float p_value) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_FLOAT, false);
    m_float = p_value;
    return true;
}

bool DynamicVariable::SetString(const std::string& p_value) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_STRING, false);
    m_string = p_value;
    return true;
}

bool DynamicVariable::SetString(std::string_view p_value) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_STRING, false);
    m_string = p_value;
    return true;
}

bool DynamicVariable::SetVector2f(float p_x, float p_y) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_VEC2, false);
    m_vec.x = p_x;
    m_vec.y = p_y;
    return true;
}

bool DynamicVariable::SetVector3f(float p_x, float p_y, float p_z) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_VEC3, false);
    m_vec.x = p_x;
    m_vec.y = p_y;
    m_vec.z = p_z;
    return true;
}

bool DynamicVariable::SetVector4f(float p_x, float p_y, float p_z, float p_w) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_VEC4, false);
    m_vec.x = p_x;
    m_vec.y = p_y;
    m_vec.z = p_z;
    m_vec.w = p_w;
    return true;
}

bool DynamicVariable::SetVector2i(int p_x, int p_y) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_IVEC2, false);
    m_ivec.x = p_x;
    m_ivec.y = p_y;
    return true;
}

bool DynamicVariable::SetVector3i(int p_x, int p_y, int p_z) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_IVEC3, false);
    m_ivec.x = p_x;
    m_ivec.y = p_y;
    m_ivec.z = p_z;
    return true;
}

bool DynamicVariable::SetVector4i(int p_x, int p_y, int p_z, int p_w) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_IVEC4, false);
    m_ivec.x = p_x;
    m_ivec.y = p_y;
    m_ivec.z = p_z;
    m_ivec.w = p_w;
    return true;
}

std::string DynamicVariable::ValueToString() const {
    switch (m_type) {
        case VARIANT_TYPE_INT:
            return std::format("{}", m_int);
        case VARIANT_TYPE_FLOAT:
            return std::format("{}", m_float);
        case VARIANT_TYPE_STRING:
            return std::format("\"{}\"", m_string);
        case VARIANT_TYPE_VEC2:
            return std::format("{} {}", m_vec.x, m_vec.y);
        case VARIANT_TYPE_IVEC2:
            return std::format("{} {}", m_ivec.x, m_ivec.y);
        case VARIANT_TYPE_VEC3:
            return std::format("{} {} {}", m_vec.x, m_vec.y, m_vec.z);
        case VARIANT_TYPE_IVEC3:
            return std::format("{} {} {}", m_ivec.x, m_ivec.y, m_ivec.z);
        case VARIANT_TYPE_VEC4:
            return std::format("{} {} {} {}", m_vec.x, m_vec.y, m_vec.z, m_vec.w);
        case VARIANT_TYPE_IVEC4:
            return std::format("{} {} {} {}", m_ivec.x, m_ivec.y, m_ivec.z, m_ivec.w);
        default:
            CRASH_NOW();
            return std::string{};
    }
}

void DynamicVariable::PrintValueChange(std::string_view p_source) const {
    static const char* s_names[] = {
        "",
        "int",
        "float",
        "string",
        "Vector2f",
        "Vector3f",
        "Vector4f",
        "Vector2i",
        "Vector3i",
        "Vector4i",
    };

    static_assert(array_length(s_names) == VARIANT_TYPE_MAX);

    std::string value_string = ValueToString();
    LOG_VERBOSE("[dvar] change dvar '{}'({}) to {} (source: {})", m_name, s_names[m_type], value_string, p_source);
}

DynamicVariable* DynamicVariable::FindDvar(const std::string& p_name) {
    auto it = s_map.find(p_name);
    if (it == s_map.end()) {
        return nullptr;
    }
    return it->second;
}

void DynamicVariable::RegisterDvar(std::string_view p_key, DynamicVariable* p_dvar) {
    const std::string keyStr(p_key);
    auto it = s_map.find(keyStr);
    if (it != s_map.end()) {
        LOG_ERROR("duplicated dvar {} detected", p_key);
    }

    p_dvar->m_name = p_key;

    s_map.insert(std::make_pair(keyStr, p_dvar));
}

}  // namespace my

#endif
