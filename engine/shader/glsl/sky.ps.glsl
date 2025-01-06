/// File: sky.ps.glsl
#include "../cbuffer.hlsl.h"
#include "../sky.hlsl.h"

in vec3 pass_pos;
out vec4 out_color;

const float Br = 0.0025f;
const float Bm = 0.0003f;
const float g = 0.9800f;
const vec3 nitrogen = vec3(0.650, 0.570, 0.475);
const vec3 Kr = Br / pow(nitrogen, vec3(4.0));
const vec3 Km = Bm / pow(nitrogen, vec3(0.84));

#if 0
float fbm(vec3 p) {
    const mat3 m = mat3(0.0, 1.60, 1.20, -1.6, 0.72, -0.96, -1.2, -0.96, 1.28);
    float f = 0.0;
    f += noise(p) / 2;
    p = m * p * 1.1;
    f += noise(p) / 4;
    p = m * p * 1.2;
    f += noise(p) / 6;
    p = m * p * 1.3;
    f += noise(p) / 12;
    p = m * p * 1.4;
    f += noise(p) / 24;
    return f;
}
#endif

void main() {
    if (pass_pos.y < 0.0f) {
        discard;
    }

    const vec3 sun = c_sunPosition;

    // Atmosphere Scattering
    float mu = dot(normalize(pass_pos), normalize(sun));
    float rayleigh = 3.0 / (8.0 * 3.14) * (1.0 + mu * mu);
    vec3 mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5)) / (Br + Bm);

    vec3 day_extinction = exp(-exp(-((pass_pos.y + sun.y * 4.0) * (exp(-pass_pos.y * 16.0) + 0.1) / 80.0) / Br) * (exp(-pass_pos.y * 16.0) + 0.1) * Kr / Br) * exp(-pass_pos.y * exp(-pass_pos.y * 8.0) * 4.0) * exp(-pass_pos.y * 2.0) * 4.0;
    vec3 night_extinction = vec3(1.0 - exp(sun.y)) * 0.2;
    vec3 extinction = mix(day_extinction, night_extinction, -sun.y * 0.2 + 0.5);
    vec3 color = rayleigh * mie * extinction;

#if 0
    // @TODO: make uniform
    float time = float(c_frameIndex) * 0.01f;
    const float cirrus = 0.4f;
    const float cumulus = 0.8f;
    // Cirrus Clouds
    float density = smoothstep(1.0 - cirrus, 1.0, fbm(pass_pos.xyz / pass_pos.y * 2.0 + time * 0.05)) * 0.3;
    color = mix(color.rgb, extinction * 4.0, density * max(pass_pos.y, 0.0));

    // Cumulus Clouds
    for (int i = 0; i < 3; i++) {
        float density = smoothstep(1.0 - cumulus, 1.0, fbm((0.7 + float(i) * 0.01) * pass_pos.xyz / pass_pos.y + time * 0.3));
        color = mix(color.rgb, extinction * density * 5.0, min(density, 1.0) * max(pass_pos.y, 0.0));
    }
#endif

    // Dithering Noise
    color += noise(pass_pos * 1000) * 0.01;
    out_color = vec4(color, 1.0f);
}
