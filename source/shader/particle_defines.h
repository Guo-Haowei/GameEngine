#ifndef PARTICLE_DEFINES_INCLUDE
#define PARTICLE_DEFINES_INCLUDE

#if defined(HLSL_LANG)
#error SBUFFER NOT DEFINED
#else
#define SBUFFER(NAME, REG) layout(std430, binding = REG) buffer NAME
#endif

struct Particle {
    vec4 velocity;
    vec3 position;
    float scale;
};

SBUFFER(ParticleData, 0) {
    Particle globalParticles[];
};

#endif
