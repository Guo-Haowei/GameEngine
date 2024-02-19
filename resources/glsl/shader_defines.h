#ifndef SHADER_DEFINES_INCLUDED
#define SHADER_DEFINES_INCLUDED

#ifndef MY_PI
#define MY_PI 3.141592653589793
#endif

#ifndef MY_TWO_PI
#define MY_TWO_PI 6.283185307179586
#endif

// light type
#define LIGHT_TYPE_OMNI  0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT  2
#define LIGHT_TYPE_AREA  3
#define LIGHT_TYPE_MAX   4

#define NUM_LIGHT_MAX       16
#define NUM_BONE_MAX        128
#define NUM_SSAO_KERNEL_MAX 64
#define NUM_CASCADE_MAX     4
#endif
