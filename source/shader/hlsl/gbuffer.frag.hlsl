#include "hlsl/vsoutput.hlsl"

struct ps_output {
    float4 base_color : SV_TARGET0;
    float4 position : SV_TARGET1;
    float4 normal : SV_TARGET2;
    float4 out_emissive_roughness_metallic : SV_TARGET3;
};

ps_output main(vsoutput_mesh input) {
    float3 N = normalize(input.normal);
    float3 color = N + float3(1.0, 1.0, 1.0);
    color = 0.5 * color;

    ps_output output;
    output.normal = float4(N, 1.0);
    return output;
}
