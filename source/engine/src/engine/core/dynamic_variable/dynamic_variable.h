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

    void RegisterInt(std::string_view p_key, int p_value);
    void RegisterFloat(std::string_view p_key, float p_value);
    void RegisterString(std::string_view p_key, std::string_view p_value);
    void RegisterVec2(std::string_view p_key, float p_x, float p_y);
    void RegisterVec3(std::string_view p_key, float p_x, float p_y, float p_z);
    void RegisterVec4(std::string_view p_key, float p_x, float p_y, float p_z, float p_w);
    void RegisterIvec2(std::string_view p_key, int p_x, int p_y);
    void RegisterIvec3(std::string_view p_key, int p_x, int p_y, int p_z);
    void RegisterIvec4(std::string_view p_key, int p_x, int p_y, int p_z, int p_w);

    int AsInt() const;
    float AsFloat() const;
    const std::string& AsString() const;
    vec2 AsVec2() const;
    vec3 AsVec3() const;
    vec4 AsVec4() const;
    ivec2 AsIvec2() const;
    ivec3 AsIvec3() const;
    ivec4 AsIvec4() const;
    void* AsPointer();

    bool SetInt(int p_value);
    bool SetFloat(float p_value);
    bool SetString(const std::string& p_value);
    bool SetString(std::string_view p_value);
    bool SetVec2(float p_x, float p_y);
    bool SetVec3(float p_x, float p_y, float p_z);
    bool SetVec4(float p_x, float p_y, float p_z, float p_w);
    bool SetIvec2(int p_x, int p_y);
    bool SetIvec3(int p_x, int p_y, int p_z);
    bool SetIvec4(int p_x, int p_y, int p_z, int p_w);

    void SetFlag(uint32_t p_flag) { m_flags |= p_flag; }
    void UnsetFlag(uint32_t p_flag) { m_flags &= ~p_flag; }

    std::string ValueToString() const;
    void PrintValueChange(std::string_view p_source) const;

    VariantType GetType() const { return m_type; }
    const char* GetDesc() const { return m_desc; }
    uint32_t GetFlags() const { return m_flags; }

    static DynamicVariable* FindDvar(const std::string& p_name);
    static void RegisterDvar(std::string_view p_key, DynamicVariable* p_dvar);

private:
    const VariantType m_type;
    const char* m_desc;
    uint32_t m_flags;

    struct Vec4f {
        float x, y, z, w;
    };

    struct Vec4i {
        int x, y, z, w;
    };

    union {
        int m_int;
        float m_float;
        Vec4f m_vec;
        Vec4i m_ivec;
    };
    std::string m_string;
    std::string m_name;

    inline static std::unordered_map<std::string, DynamicVariable*> s_map;
    friend class DynamicVariableManager;
};

}  // namespace my

#define DVAR_GET_BOOL(name)    (!!(DVAR_##name).AsInt())
#define DVAR_GET_INT(name)     (DVAR_##name).AsInt()
#define DVAR_GET_FLOAT(name)   (DVAR_##name).AsFloat()
#define DVAR_GET_STRING(name)  (DVAR_##name).AsString()
#define DVAR_GET_VEC2(name)    (DVAR_##name).AsVec2()
#define DVAR_GET_VEC3(name)    (DVAR_##name).AsVec3()
#define DVAR_GET_VEC4(name)    (DVAR_##name).AsVec4()
#define DVAR_GET_IVEC2(name)   (DVAR_##name).AsIvec2()
#define DVAR_GET_IVEC3(name)   (DVAR_##name).AsIvec3()
#define DVAR_GET_IVEC4(name)   (DVAR_##name).AsIvec4()
#define DVAR_GET_POINTER(name) (DVAR_##name).AsPointer()

#define DVAR_SET_BOOL(name, value)       (DVAR_##name).SetInt(!!(value))
#define DVAR_SET_INT(name, value)        (DVAR_##name).SetInt(value)
#define DVAR_SET_FLOAT(name, value)      (DVAR_##name).SetFloat(value)
#define DVAR_SET_STRING(name, value)     (DVAR_##name).SetString(value)
#define DVAR_SET_VEC2(name, x, y)        (DVAR_##name).SetVec2(x, y)
#define DVAR_SET_VEC3(name, x, y, z)     (DVAR_##name).SetVec3(x, y, z)
#define DVAR_SET_VEC4(name, x, y, z, w)  (DVAR_##name).SetVec4(x, y, z, w)
#define DVAR_SET_IVEC2(name, x, y)       (DVAR_##name).SetIvec2(x, y)
#define DVAR_SET_IVEC3(name, x, y, z)    (DVAR_##name).SetIvec3(x, y, z)
#define DVAR_SET_IVEC4(name, x, y, z, w) (DVAR_##name).SetIvec4(x, y, z, w)
