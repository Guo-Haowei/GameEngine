/// File: shared_diffuse_irradiance.h
#define SAMPLE_STEP 0.025f

#if defined(GLSL_LANG)
void main() {
    Vector3f N = normalize(out_var_POSITION);
#else
float4 main(VS_OUTPUT_POSITION input)
    : SV_TARGET {
    Vector3f N = normalize(input.world_position);
#endif
    Vector3f up = Vector3f(0.0, 1.0, 0.0);
    Vector3f right = cross(up, N);
    up = cross(N, right);

    Vector3f irradiance = Vector3f(0.0, 0.0, 0.0);
    float samples = 0.0;

    for (float phi = 0.0; phi < 2.0 * MY_PI; phi += SAMPLE_STEP) {
        for (float theta = 0.0; theta < 0.5 * MY_PI; theta += SAMPLE_STEP) {
            float xdir = sin(theta) * cos(phi);
            float ydir = sin(theta) * sin(phi);
            float zdir = cos(theta);
            Vector3f sample_dir = xdir * right + ydir * up + zdir * N;
#if defined(GLSL_LANG)
            irradiance += textureLod(t_Skybox, sample_dir, 0.0).rgb * cos(theta) * sin(theta);
#else
            irradiance += t_Skybox.SampleLevel(s_cubemapClampSampler, sample_dir, 0.0).rgb * cos(theta) * sin(theta);
#endif
            samples += 1.0;
        }
    }

    irradiance = MY_PI * irradiance * (1.0 / samples);
#if defined(GLSL_LANG)
    out_color = Vector4f(irradiance, 1.0);
#else
    return Vector4f(irradiance, 1.0);
#endif
}