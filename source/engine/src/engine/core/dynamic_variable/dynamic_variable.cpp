#include "dynamic_variable.h"

namespace my {

void DynamicVariable::registerInt(std::string_view p_key, int p_value) {
    m_int = p_value;
    registerDvar(p_key, this);
}

void DynamicVariable::registerFloat(std::string_view p_key, float p_value) {
    m_float = p_value;
    registerDvar(p_key, this);
}

void DynamicVariable::registerString(std::string_view p_key, std::string_view p_value) {
    m_string = p_value;
    registerDvar(p_key, this);
}

void DynamicVariable::registerVec2(std::string_view p_key, float p_x, float p_y) {
    m_vec.x = p_x;
    m_vec.y = p_y;
    registerDvar(p_key, this);
}

void DynamicVariable::registerVec3(std::string_view p_key, float p_x, float p_y, float p_z) {
    m_vec.x = p_x;
    m_vec.y = p_y;
    m_vec.z = p_z;
    registerDvar(p_key, this);
}

void DynamicVariable::registerVec4(std::string_view p_key, float p_x, float p_y, float p_z, float p_w) {
    m_vec.x = p_x;
    m_vec.y = p_y;
    m_vec.z = p_z;
    m_vec.w = p_w;
    registerDvar(p_key, this);
}

void DynamicVariable::registerIvec2(std::string_view p_key, int p_x, int p_y) {
    m_ivec.x = p_x;
    m_ivec.y = p_y;
    registerDvar(p_key, this);
}

void DynamicVariable::registerIvec3(std::string_view p_key, int p_x, int p_y, int p_z) {
    m_ivec.x = p_x;
    m_ivec.y = p_y;
    m_ivec.z = p_z;
    registerDvar(p_key, this);
}

void DynamicVariable::registerIvec4(std::string_view p_key, int p_x, int p_y, int p_z, int p_w) {
    m_ivec.x = p_x;
    m_ivec.y = p_y;
    m_ivec.z = p_z;
    m_ivec.w = p_w;
    registerDvar(p_key, this);
}

int DynamicVariable::asInt() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_INT);
    return m_int;
}

float DynamicVariable::asFloat() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_FLOAT);
    return m_float;
}

const std::string& DynamicVariable::asString() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_STRING);
    return m_string;
}

vec2 DynamicVariable::asVec2() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_VEC2);
    return vec2(m_vec.x, m_vec.y);
}

vec3 DynamicVariable::asVec3() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_VEC3);
    return vec3(m_vec.x, m_vec.y, m_vec.z);
}

vec4 DynamicVariable::asVec4() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_VEC4);
    return vec4(m_vec.x, m_vec.y, m_vec.z, m_vec.w);
}

ivec2 DynamicVariable::asIvec2() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_IVEC2);
    return ivec2(m_ivec.x, m_ivec.y);
}

ivec3 DynamicVariable::asIvec3() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_IVEC3);
    return ivec3(m_ivec.x, m_ivec.y, m_ivec.z);
}

ivec4 DynamicVariable::asIvec4() const {
    DEV_ASSERT(m_type == VARIANT_TYPE_IVEC4);
    return ivec4(m_ivec.x, m_ivec.y, m_ivec.z, m_ivec.w);
}

void* DynamicVariable::asPointer() {
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

bool DynamicVariable::setInt(int p_value) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_INT, false);
    m_int = p_value;
    return true;
}

bool DynamicVariable::setFloat(float p_value) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_FLOAT, false);
    m_float = p_value;
    return true;
}

bool DynamicVariable::setString(const std::string& p_value) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_STRING, false);
    m_string = p_value;
    return true;
}

bool DynamicVariable::setString(std::string_view p_value) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_STRING, false);
    m_string = p_value;
    return true;
}

bool DynamicVariable::setVec2(float p_x, float p_y) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_VEC2, false);
    m_vec.x = p_x;
    m_vec.y = p_y;
    return true;
}

bool DynamicVariable::setVec3(float p_x, float p_y, float p_z) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_VEC3, false);
    m_vec.x = p_x;
    m_vec.y = p_y;
    m_vec.z = p_z;
    return true;
}

bool DynamicVariable::setVec4(float p_x, float p_y, float p_z, float p_w) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_VEC4, false);
    m_vec.x = p_x;
    m_vec.y = p_y;
    m_vec.z = p_z;
    m_vec.w = p_w;
    return true;
}

bool DynamicVariable::setIvec2(int p_x, int p_y) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_IVEC2, false);
    m_ivec.x = p_x;
    m_ivec.y = p_y;
    return true;
}

bool DynamicVariable::setIvec3(int p_x, int p_y, int p_z) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_IVEC3, false);
    m_ivec.x = p_x;
    m_ivec.y = p_y;
    m_ivec.z = p_z;
    return true;
}

bool DynamicVariable::setIvec4(int p_x, int p_y, int p_z, int p_w) {
    ERR_FAIL_COND_V(m_type != VARIANT_TYPE_IVEC4, false);
    m_ivec.x = p_x;
    m_ivec.y = p_y;
    m_ivec.z = p_z;
    m_ivec.w = p_w;
    return true;
}

std::string DynamicVariable::valueToString() const {
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

void DynamicVariable::printValueChange(std::string_view p_source) const {
    static const char* s_names[] = {
        "",
        "int",
        "float",
        "string",
        "vec2",
        "vec3",
        "vec4",
        "ivec2",
        "ivec3",
        "ivec4",
    };

    static_assert(array_length(s_names) == VARIANT_TYPE_MAX);

    std::string value_string = valueToString();
    LOG_VERBOSE("[dvar] change dvar '{}'({}) to {} (source: {})", m_name, s_names[m_type], value_string, p_source);
}

DynamicVariable* DynamicVariable::findDvar(const std::string& p_name) {
    auto it = s_map.find(p_name);
    if (it == s_map.end()) {
        return nullptr;
    }
    return it->second;
}

void DynamicVariable::registerDvar(std::string_view p_key, DynamicVariable* p_dvar) {
    const std::string keyStr(p_key);
    auto it = s_map.find(keyStr);
    if (it != s_map.end()) {
        LOG_ERROR("duplicated dvar {} detected", p_key);
    }

    p_dvar->m_name = p_key;

    s_map.insert(std::make_pair(keyStr, p_dvar));
}

}  // namespace my
