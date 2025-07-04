#pragma once
#include "engine/math/vector.h"

#define ENABLE_DVAR USE_IF(!USING(PLATFORM_WASM))

#if USING(ENABLE_DVAR)
enum DvarFlags : uint32_t {
    DVAR_FLAG_NONE = BIT(0),
    DVAR_FLAG_CACHE = BIT(1),
    DVAR_FLAG_OVERRIDEN = BIT(2),
};
DEFINE_ENUM_BITWISE_OPERATIONS(DvarFlags);

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
    explicit DynamicVariable(VariantType p_type, DvarFlags p_flags, const char* p_desc);

    void RegisterInt(std::string_view p_key, int p_value);
    void RegisterFloat(std::string_view p_key, float p_value);
    void RegisterString(std::string_view p_key, std::string_view p_value);
    void RegisterVector2f(std::string_view p_key, float p_x, float p_y);
    void RegisterVector3f(std::string_view p_key, float p_x, float p_y, float p_z);
    void RegisterVector4f(std::string_view p_key, float p_x, float p_y, float p_z, float p_w);
    void RegisterVector2i(std::string_view p_key, int p_x, int p_y);
    void RegisterVector3i(std::string_view p_key, int p_x, int p_y, int p_z);
    void RegisterVector4i(std::string_view p_key, int p_x, int p_y, int p_z, int p_w);

    [[nodiscard]] int AsInt() const;
    [[nodiscard]] float AsFloat() const;
    [[nodiscard]] const std::string& AsString() const;
    [[nodiscard]] Vector2f AsVector2f() const;
    [[nodiscard]] Vector3f AsVector3f() const;
    [[nodiscard]] Vector4f AsVector4f() const;
    [[nodiscard]] Vector2i AsVector2i() const;
    [[nodiscard]] Vector3i AsVector3i() const;
    [[nodiscard]] Vector4i AsVector4i() const;
    [[nodiscard]] void* AsPointer();

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

    void SetFlag(DvarFlags p_flag) { m_flags |= p_flag; }
    void UnsetFlag(DvarFlags p_flag) { m_flags &= ~p_flag; }

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
    DvarFlags m_flags;

    union {
        int m_int;
        float m_float;
        Vector4f m_vec;
        Vector4i m_ivec;
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

#else

#define DVAR_GET_BOOL(name)    !!(DVAR_##name)
#define DVAR_GET_INT(name)     (DVAR_##name)
#define DVAR_GET_FLOAT(name)   (DVAR_##name)
#define DVAR_GET_STRING(name)  (DVAR_##name)
#define DVAR_GET_VEC2(name)    (DVAR_##name)
#define DVAR_GET_VEC3(name)    (DVAR_##name)
#define DVAR_GET_VEC4(name)    (DVAR_##name)
#define DVAR_GET_IVEC2(name)   (DVAR_##name)
#define DVAR_GET_IVEC3(name)   (DVAR_##name)
#define DVAR_GET_IVEC4(name)   (DVAR_##name)
#define DVAR_GET_POINTER(name) ((void*)&(DVAR_##name))

#define DVAR_SET_BOOL(name, value)       ((void)0)
#define DVAR_SET_INT(name, value)        ((void)0)
#define DVAR_SET_FLOAT(name, value)      ((void)0)
#define DVAR_SET_STRING(name, value)     ((void)0)
#define DVAR_SET_VEC2(name, x, y)        ((void)0)
#define DVAR_SET_VEC3(name, x, y, z)     ((void)0)
#define DVAR_SET_VEC4(name, x, y, z, w)  ((void)0)
#define DVAR_SET_IVEC2(name, x, y)       ((void)0)
#define DVAR_SET_IVEC3(name, x, y, z)    ((void)0)
#define DVAR_SET_IVEC4(name, x, y, z, w) ((void)0)

#endif
