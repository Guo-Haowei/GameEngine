#pragma once
#include "core/math/geomath.h"

enum DvarFlags {
    DVAR_FLAG_NONE = BIT(0),
    DVAR_FLAG_CACHE = BIT(1),
};

namespace my {

enum VariantType {
    VARIANT_TYPE_INVALID = 0,
    VARIANT_TYPE_INT,
    VARIANT_TYPE_FLOAT,
    VARIANT_TYPE_STRING,
    VARIANT_TYPE_VEC2,
    VARIANT_TYPE_VEC3,
    VARIANT_TYPE_VEC4,
    VARIANT_TYPE_IVEC2,
    VARIANT_TYPE_IVEC3,
    VARIANT_TYPE_IVEC4,
    VARIANT_TYPE_MAX,
};

class DynamicVariable {
public:
    DynamicVariable(VariantType p_type, uint32_t p_flags, const char* p_desc)
        : m_type(p_type), m_desc(p_desc), m_flags(p_flags), m_int(0) {}

    void registerInt(std::string_view p_key, int p_value);
    void registerFloat(std::string_view p_key, float p_value);
    void registerString(std::string_view p_key, std::string_view p_value);
    void registerVec2(std::string_view p_key, float p_x, float p_y);
    void registerVec3(std::string_view p_key, float p_x, float p_y, float p_z);
    void registerVec4(std::string_view p_key, float p_x, float p_y, float p_z, float p_w);
    void registerIvec2(std::string_view p_key, int p_x, int p_y);
    void registerIvec3(std::string_view p_key, int p_x, int p_y, int p_z);
    void registerIvec4(std::string_view p_key, int p_x, int p_y, int p_z, int p_w);

    int asInt() const;
    float asFloat() const;
    const std::string& asString() const;
    vec2 asVec2() const;
    vec3 asVec3() const;
    vec4 asVec4() const;
    ivec2 asIvec2() const;
    ivec3 asIvec3() const;
    ivec4 asIvec4() const;
    void* asPointer();

    bool setInt(int p_value);
    bool setFloat(float p_value);
    bool setString(const std::string& p_value);
    bool setString(std::string_view p_value);
    bool setVec2(float p_x, float p_y);
    bool setVec3(float p_x, float p_y, float p_z);
    bool setVec4(float p_x, float p_y, float p_z, float p_w);
    bool setIvec2(int p_x, int p_y);
    bool setIvec3(int p_x, int p_y, int p_z);
    bool setIvec4(int p_x, int p_y, int p_z, int p_w);

    void setFlag(uint32_t p_flag) { m_flags |= p_flag; }
    void unsetFlag(uint32_t p_flag) { m_flags &= ~p_flag; }

    std::string valueToString() const;
    void printValueChange(std::string_view p_source) const;

    VariantType getType() const { return m_type; }
    const char* getDesc() const { return m_desc; }
    uint32_t getFlags() const { return m_flags; }

    static DynamicVariable* findDvar(const std::string& p_name);
    static void registerDvar(std::string_view p_key, DynamicVariable* p_dvar);

private:
    const VariantType m_type;
    const char* m_desc;
    uint32_t m_flags;

    union {
        int m_int;
        float m_float;
        struct {
            float x, y, z, w;
        } m_vec;
        struct {
            int x, y, z, w;
        } m_ivec;
    };
    std::string m_string;
    std::string m_name;

    inline static std::unordered_map<std::string, DynamicVariable*> s_map;
    friend class DynamicVariableManager;
};

}  // namespace my

#define DVAR_GET_BOOL(name)    (!!(DVAR_##name).asInt())
#define DVAR_GET_INT(name)     (DVAR_##name).asInt()
#define DVAR_GET_FLOAT(name)   (DVAR_##name).asFloat()
#define DVAR_GET_STRING(name)  (DVAR_##name).asString()
#define DVAR_GET_VEC2(name)    (DVAR_##name).asVec2()
#define DVAR_GET_VEC3(name)    (DVAR_##name).asVec3()
#define DVAR_GET_VEC4(name)    (DVAR_##name).asVec4()
#define DVAR_GET_IVEC2(name)   (DVAR_##name).asIvec2()
#define DVAR_GET_IVEC3(name)   (DVAR_##name).asIvec3()
#define DVAR_GET_IVEC4(name)   (DVAR_##name).asIvec4()
#define DVAR_GET_POINTER(name) (DVAR_##name).asPointer()

#define DVAR_SET_BOOL(name, value)       (DVAR_##name).setInt(!!(value))
#define DVAR_SET_INT(name, value)        (DVAR_##name).setInt(value)
#define DVAR_SET_FLOAT(name, value)      (DVAR_##name).setFloat(value)
#define DVAR_SET_STRING(name, value)     (DVAR_##name).setString(value)
#define DVAR_SET_VEC2(name, x, y)        (DVAR_##name).setVec2(x, y)
#define DVAR_SET_VEC3(name, x, y, z)     (DVAR_##name).setVec3(x, y, z)
#define DVAR_SET_VEC4(name, x, y, z, w)  (DVAR_##name).setVec4(x, y, z, w)
#define DVAR_SET_IVEC2(name, x, y)       (DVAR_##name).setIvec2(x, y)
#define DVAR_SET_IVEC3(name, x, y, z)    (DVAR_##name).setIvec3(x, y, z)
#define DVAR_SET_IVEC4(name, x, y, z, w) (DVAR_##name).setIvec4(x, y, z, w)
