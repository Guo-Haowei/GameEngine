#include "dynamic_variable.h"

#if USING(ENABLE_DVAR)
#if defined(DEFINE_DVAR)
#define DVAR_BOOL(name, flags, desc, value)       my::DynamicVariable DVAR_##name(my::VARIANT_TYPE_INT, flags, desc)
#define DVAR_INT(name, flags, desc, value)        my::DynamicVariable DVAR_##name(my::VARIANT_TYPE_INT, flags, desc)
#define DVAR_FLOAT(name, flags, desc, value)      my::DynamicVariable DVAR_##name(my::VARIANT_TYPE_FLOAT, flags, desc)
#define DVAR_STRING(name, flags, desc, value)     my::DynamicVariable DVAR_##name(my::VARIANT_TYPE_STRING, flags, desc)
#define DVAR_VEC2(name, flags, desc, x, y)        my::DynamicVariable DVAR_##name(my::VARIANT_TYPE_VEC2, flags, desc)
#define DVAR_VEC3(name, flags, desc, x, y, z)     my::DynamicVariable DVAR_##name(my::VARIANT_TYPE_VEC3, flags, desc)
#define DVAR_VEC4(name, flags, desc, x, y, z, w)  my::DynamicVariable DVAR_##name(my::VARIANT_TYPE_VEC4, flags, desc)
#define DVAR_IVEC2(name, flags, desc, x, y)       my::DynamicVariable DVAR_##name(my::VARIANT_TYPE_IVEC2, flags, desc)
#define DVAR_IVEC3(name, flags, desc, x, y, z)    my::DynamicVariable DVAR_##name(my::VARIANT_TYPE_IVEC3, flags, desc)
#define DVAR_IVEC4(name, flags, desc, x, y, z, w) my::DynamicVariable DVAR_##name(my::VARIANT_TYPE_IVEC4, flags, desc)
#elif defined(REGISTER_DVAR)
#define DVAR_BOOL(name, flags, desc, value)       (DVAR_##name).RegisterInt(#name, !!(value))
#define DVAR_INT(name, flags, desc, value)        (DVAR_##name).RegisterInt(#name, value)
#define DVAR_FLOAT(name, flags, desc, value)      (DVAR_##name).RegisterFloat(#name, value)
#define DVAR_STRING(name, flags, desc, value)     (DVAR_##name).RegisterString(#name, value)
#define DVAR_VEC2(name, flags, desc, x, y)        (DVAR_##name).RegisterVector2f(#name, x, y)
#define DVAR_VEC3(name, flags, desc, x, y, z)     (DVAR_##name).RegisterVector3f(#name, x, y, z)
#define DVAR_VEC4(name, flags, desc, x, y, z, w)  (DVAR_##name).RegisterVector4f(#name, x, y, z, w)
#define DVAR_IVEC2(name, flags, desc, x, y)       (DVAR_##name).RegisterVector2i(#name, x, y)
#define DVAR_IVEC3(name, flags, desc, x, y, z)    (DVAR_##name).RegisterVector3i(#name, x, y, z)
#define DVAR_IVEC4(name, flags, desc, x, y, z, w) (DVAR_##name).RegisterVector4i(#name, x, y, z, w)
#else
#define DVAR_BOOL(name, ...)   extern my::DynamicVariable DVAR_##name
#define DVAR_INT(name, ...)    extern my::DynamicVariable DVAR_##name
#define DVAR_FLOAT(name, ...)  extern my::DynamicVariable DVAR_##name
#define DVAR_STRING(name, ...) extern my::DynamicVariable DVAR_##name
#define DVAR_VEC2(name, ...)   extern my::DynamicVariable DVAR_##name
#define DVAR_VEC3(name, ...)   extern my::DynamicVariable DVAR_##name
#define DVAR_VEC4(name, ...)   extern my::DynamicVariable DVAR_##name
#define DVAR_IVEC2(name, ...)  extern my::DynamicVariable DVAR_##name
#define DVAR_IVEC3(name, ...)  extern my::DynamicVariable DVAR_##name
#define DVAR_IVEC4(name, ...)  extern my::DynamicVariable DVAR_##name
#endif
#else
#if defined(DEFINE_DVAR)
#define DVAR_BOOL(name, flags, desc, value)       int DVAR_##name = { value }
#define DVAR_INT(name, flags, desc, value)        int DVAR_##name = { value }
#define DVAR_FLOAT(name, flags, desc, value)      float DVAR_##name = { value }
#define DVAR_STRING(name, flags, desc, value)     std::string DVAR_##name = { value }
#define DVAR_VEC2(name, flags, desc, x, y)        ::my::Vector2f DVAR_##name = { x, y }
#define DVAR_VEC3(name, flags, desc, x, y, z)     ::my::Vector3f DVAR_##name = { x, y, z }
#define DVAR_VEC4(name, flags, desc, x, y, z, w)  ::my::Vector4f DVAR_##name = { x, y, z, w }
#define DVAR_IVEC2(name, flags, desc, x, y)       ::my::Vector2i DVAR_##name = { x, y }
#define DVAR_IVEC3(name, flags, desc, x, y, z)    ::my::Vector3i DVAR_##name = { x, y, z }
#define DVAR_IVEC4(name, flags, desc, x, y, z, w) ::my::Vector4i DVAR_##name = { x, y, z, w }
#elif defined(REGISTER_DVAR)
#define DVAR_BOOL(name, flags, desc, value)       ((void)0)
#define DVAR_INT(name, flags, desc, value)        ((void)0)
#define DVAR_FLOAT(name, flags, desc, value)      ((void)0)
#define DVAR_STRING(name, flags, desc, value)     ((void)0)
#define DVAR_VEC2(name, flags, desc, x, y)        ((void)0)
#define DVAR_VEC3(name, flags, desc, x, y, z)     ((void)0)
#define DVAR_VEC4(name, flags, desc, x, y, z, w)  ((void)0)
#define DVAR_IVEC2(name, flags, desc, x, y)       ((void)0)
#define DVAR_IVEC3(name, flags, desc, x, y, z)    ((void)0)
#define DVAR_IVEC4(name, flags, desc, x, y, z, w) ((void)0)
#else
#define DVAR_BOOL(name, ...)   extern int DVAR_##name
#define DVAR_INT(name, ...)    extern int DVAR_##name
#define DVAR_FLOAT(name, ...)  extern float DVAR_##name
#define DVAR_STRING(name, ...) extern std::string DVAR_##name
#define DVAR_VEC2(name, ...)   extern ::my::Vector2f DVAR_##name
#define DVAR_VEC3(name, ...)   extern ::my::Vector3f DVAR_##name
#define DVAR_VEC4(name, ...)   extern ::my::Vector4f DVAR_##name
#define DVAR_IVEC2(name, ...)  extern ::my::Vector2i DVAR_##name
#define DVAR_IVEC3(name, ...)  extern ::my::Vector3i DVAR_##name
#define DVAR_IVEC4(name, ...)  extern ::my::Vector4i DVAR_##name
#endif
#endif
