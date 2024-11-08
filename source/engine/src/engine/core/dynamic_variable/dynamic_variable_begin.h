#include "dynamic_variable.h"

// clang-format off
#if defined(DEFINE_DVAR)
#define DVAR_BOOL(name, flags, desc, value)			my::DynamicVariable DVAR_##name { my::VARIANT_TYPE_INT,		flags, desc }
#define DVAR_INT(name, flags, desc, value)			my::DynamicVariable DVAR_##name { my::VARIANT_TYPE_INT,		flags, desc }
#define DVAR_FLOAT(name, flags, desc, value)		my::DynamicVariable DVAR_##name { my::VARIANT_TYPE_FLOAT,	flags, desc }
#define DVAR_STRING(name, flags, desc, value)		my::DynamicVariable DVAR_##name { my::VARIANT_TYPE_STRING,	flags, desc }
#define DVAR_VEC2(name, flags, desc, x, y)			my::DynamicVariable DVAR_##name { my::VARIANT_TYPE_VEC2,	flags, desc }
#define DVAR_VEC3(name, flags, desc, x, y, z)		my::DynamicVariable DVAR_##name { my::VARIANT_TYPE_VEC3,	flags, desc }
#define DVAR_VEC4(name, flags, desc, x, y, z, w)	my::DynamicVariable DVAR_##name { my::VARIANT_TYPE_VEC4,	flags, desc }
#define DVAR_IVEC2(name, flags, desc, x, y)			my::DynamicVariable DVAR_##name { my::VARIANT_TYPE_IVEC2,	flags, desc }
#define DVAR_IVEC3(name, flags, desc, x, y, z)		my::DynamicVariable DVAR_##name { my::VARIANT_TYPE_IVEC3,	flags, desc }
#define DVAR_IVEC4(name, flags, desc, x, y, z, w)	my::DynamicVariable DVAR_##name { my::VARIANT_TYPE_IVEC4,	flags, desc }
#elif defined(REGISTER_DVAR)
#define DVAR_BOOL(name, flags, desc, value)			(DVAR_##name).RegisterInt		(#name, !!(value))
#define DVAR_INT(name, flags, desc, value)			(DVAR_##name).RegisterInt		(#name, value)
#define DVAR_FLOAT(name, flags, desc, value)		(DVAR_##name).RegisterFloat		(#name, value)
#define DVAR_STRING(name, flags, desc, value)		(DVAR_##name).RegisterString	(#name, value)
#define DVAR_VEC2(name, flags, desc, x, y)			(DVAR_##name).RegisterVec2		(#name, x, y)
#define DVAR_VEC3(name, flags, desc, x, y, z)		(DVAR_##name).RegisterVec3		(#name, x, y, z)
#define DVAR_VEC4(name, flags, desc, x, y, z, w)	(DVAR_##name).RegisterVec4		(#name, x, y, z, w)
#define DVAR_IVEC2(name, flags, desc, x, y)			(DVAR_##name).RegisterIvec2		(#name, x, y)
#define DVAR_IVEC3(name, flags, desc, x, y, z)		(DVAR_##name).RegisterIvec3		(#name, x, y, z)
#define DVAR_IVEC4(name, flags, desc, x, y, z, w)	(DVAR_##name).RegisterIvec4		(#name, x, y, z, w)
#else
#define DVAR_BOOL(name, ...)	extern my::DynamicVariable DVAR_##name
#define DVAR_INT(name, ...)		extern my::DynamicVariable DVAR_##name
#define DVAR_FLOAT(name, ...)	extern my::DynamicVariable DVAR_##name
#define DVAR_STRING(name, ...)	extern my::DynamicVariable DVAR_##name
#define DVAR_VEC2(name, ...)	extern my::DynamicVariable DVAR_##name
#define DVAR_VEC3(name, ...)	extern my::DynamicVariable DVAR_##name
#define DVAR_VEC4(name, ...)	extern my::DynamicVariable DVAR_##name
#define DVAR_IVEC2(name, ...)	extern my::DynamicVariable DVAR_##name
#define DVAR_IVEC3(name, ...)	extern my::DynamicVariable DVAR_##name
#define DVAR_IVEC4(name, ...)	extern my::DynamicVariable DVAR_##name
#endif
// clang-format on
