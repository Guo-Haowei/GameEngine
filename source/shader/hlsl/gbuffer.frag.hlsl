#include "hlsl/vsoutput.hlsl"

struct ps_output {
    float4 base_color : SV_TARGET0;
    float3 position : SV_TARGET1;
    float3 normal : SV_TARGET2;
    float3 out_emissive_roughness_metallic : SV_TARGET3;
};

ps_output main(vsoutput_mesh input) {
    float3 N = normalize(input.normal);
    float3 color = N + float3(1.0, 1.0, 1.0);
    color = 0.5 * color;

    ps_output output;
    output.base_color = float4(color, 1.0);
    output.position = input.world_position;
    output.normal = N;
    output.out_emissive_roughness_metallic = float3(0.0, 1.0, 0.1);
    return output;
}
