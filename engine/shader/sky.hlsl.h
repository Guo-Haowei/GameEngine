/// File: sky.hlsl.h

// @TODO: refactor
#if defined(HLSL_LANG)
#define fract(x) ((x) - floor(x))
#define mix      lerp
#endif

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

float noise(Vector3f x) {
    Vector3f f = fract(x);
    float n = dot(floor(x), Vector3f(1.0, 157.0, 113.0));
    return mix(mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
                   mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y),
               mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
                   mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y),
               f.z);
}
