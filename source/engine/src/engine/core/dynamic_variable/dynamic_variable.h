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
    void RegisterVector2f(std::string_view p_key, float p_x, float p_y);
    void RegisterVector3f(std::string_view p_key, float p_x, float p_y, float p_z);
    void RegisterVector4f(std::string_view p_key, float p_x, float p_y, float p_z, float p_w);
    void RegisterVector2i(std::string_view p_key, int p_x, int p_y);
    void RegisterVector3i(std::string_view p_key, int p_x, int p_y, int p_z);
    void RegisterVector4i(std::string_view p_key, int p_x, int p_y, int p_z, int p_w);

    int AsInt() const;
    float AsFloat() const;
    const std::string& AsString() const;
    Vector2f AsVector2f() const;
    Vector3f AsVector3f() const;
    Vector4f AsVector4f() const;
    Vector2i AsVector2i() const;
    Vector3i AsVector3i() const;
    Vector4i AsVector4i() const;
    void* AsPointer();

    bool SetInt(int p_value);
    bool SetFloat(float p_value);
    bool SetString(const std::string& p_value);
    bool SetString(std::string_view p_value);
    bool SetVector2f(float p_x, float p_y);
    bool SetVector3f(float p_x, float p_y, float p_z);
    bool SetVector4f(float p_x, float p_y, float p_z, float p_w);
    bool SetVector2i(int p_x, int p_y);
    bool SetVector3i(int p_x, int p_y, int p_z);
    bool SetVector4i(int p_x, int p_y, int p_z, int p_w);

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

    struct vector4f_t {
        float x, y, z, w;
    };

    struct vector4i_t {
        int x, y, z, w;
    };

    union {
        int m_int;
        float m_float;
        vector4f_t m_vec;
        vector4i_t m_ivec;
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
#define DVAR_GET_VEC2(name)    (DVAR_##name).AsVector2f()
#define DVAR_GET_VEC3(name)    (DVAR_##name).AsVector3f()
#define DVAR_GET_VEC4(name)    (DVAR_##name).AsVector4f()
#define DVAR_GET_IVEC2(name)   (DVAR_##name).AsVector2i()
#define DVAR_GET_IVEC3(name)   (DVAR_##name).AsVector3i()
#define DVAR_GET_IVEC4(name)   (DVAR_##name).AsVector4i()
#define DVAR_GET_POINTER(name) (DVAR_##name).AsPointer()

#define DVAR_SET_BOOL(name, value)       (DVAR_##name).SetInt(!!(value))
#define DVAR_SET_INT(name, value)        (DVAR_##name).SetInt(value)
#define DVAR_SET_FLOAT(name, value)      (DVAR_##name).SetFloat(value)
#define DVAR_SET_STRING(name, value)     (DVAR_##name).SetString(value)
#define DVAR_SET_VEC2(name, x, y)        (DVAR_##name).SetVector2f(x, y)
#define DVAR_SET_VEC3(name, x, y, z)     (DVAR_##name).SetVector3f(x, y, z)
#define DVAR_SET_VEC4(name, x, y, z, w)  (DVAR_##name).SetVector4f(x, y, z, w)
#define DVAR_SET_IVEC2(name, x, y)       (DVAR_##name).SetVector2i(x, y)
#define DVAR_SET_IVEC3(name, x, y, z)    (DVAR_##name).SetVector3i(x, y, z)
#define DVAR_SET_IVEC4(name, x, y, z, w) (DVAR_##name).SetVector4i(x, y, z, w)
