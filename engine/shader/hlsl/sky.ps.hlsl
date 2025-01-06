/// File: sky.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sky.hlsl.h"

float4 main(vsoutput_position input) : SV_TARGET {
    clip(input.world_position.y);

    const float Br = 0.0025;
    const float Bm = 0.0003;
    const float g = 0.9800;
    const Vector3f nitrogen = Vector3f(0.650, 0.570, 0.475);
    const Vector3f Kr = Br / pow(nitrogen, Vector3f(4.0, 4.0, 4.0));
    const Vector3f Km = Bm / pow(nitrogen, Vector3f(0.84, 0.84, 0.84));

    const Vector3f sun = c_sunPosition;
    const Vector3f pos = input.world_position;

    // Atmosphere Scattering
    float mu = dot(normalize(pos), normalize(sun));
    float rayleigh = 3.0 / (8.0 * 3.14) * (1.0 + mu * mu);
    Vector3f mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5)) / (Br + Bm);

    Vector3f day_extinction = exp(-exp(-((pos.y + sun.y * 4.0) * (exp(-pos.y * 16.0) + 0.1) / 80.0) / Br) * (exp(-pos.y * 16.0) + 0.1) * Kr / Br) * exp(-pos.y * exp(-pos.y * 8.0) * 4.0) * exp(-pos.y * 2.0) * 4.0;
    Vector3f night_extinction = 0.2f * (Vector3f(1.0f, 1.0f, 1.0f) - exp(sun.y));
    Vector3f extinction = mix(day_extinction, night_extinction, -sun.y * 0.2 + 0.5);
    Vector3f color = rayleigh * mie * extinction;

    color += noise(pos * 1000) * 0.01;
    return Vector4f(color, 1.0f);
}
