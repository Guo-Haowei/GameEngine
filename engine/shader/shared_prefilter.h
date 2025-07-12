/// File: shared_prefilter.h
#define SAMPLE_COUNT 1024u

#if defined(GLSL_LANG)
void main() {
    Vector3f N = normalize(out_var_POSITION);
#else
float4 main(VS_OUTPUT_POSITION input)
    : SV_TARGET {
    Vector3f N = normalize(input.world_position);
#endif

    // make the simplyfying assumption that V equals R equals the normal
    Vector3f R = N;
    Vector3f V = R;

    float roughness = c_envPassRoughness;

    Vector3f prefilteredColor = Vector3f(0.0, 0.0, 0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        Vector2f Xi = Hammersley(i, SAMPLE_COUNT);
        Vector3f H = ImportanceSampleGGX(Xi, N, roughness);
        Vector3f L = reflect(-V, H);

        float NdotL = dot(N, L);
        if (NdotL > 0.0) {
            // sample from the environment's mip level based on roughness/pdf
            float NdotH = max(dot(N, H), 0.0);
            float D = DistributionGGX(NdotH, roughness);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

            // @TODO: fix hard code
            float resolution = 512.0;  // resolution of source cubemap (per face)
            float saTexel = 4.0 * MY_PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

#if defined(GLSL_LANG)
            prefilteredColor += textureLod(t_Skybox, L, mipLevel).rgb * NdotL;
#else
            prefilteredColor += t_Skybox.SampleLevel(s_cubemapClampSampler, L, mipLevel).rgb * NdotL;
#endif
            totalWeight += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;

#if defined(GLSL_LANG)
    out_color = vec4(prefilteredColor, 1.0);
#else
    return Vector4f(prefilteredColor, 1.0);
#endif
}