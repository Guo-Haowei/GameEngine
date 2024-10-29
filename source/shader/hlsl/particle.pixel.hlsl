float4 main(float4 input
            : SV_POSITION) : SV_TARGET {
    float3 color = float3(2.0, 2.0, 0.0);
    return float4(color, 1.0);
}
