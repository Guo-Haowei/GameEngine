/// File: pass_tracer.cs.hlsl
#include "shader_defines.hlsl.h"
#include "unordered_access_defines.hlsl.h"

[numthreads(16, 16, 1)] void main(uint3 p_dispatch_thread_id : SV_DISPATCHTHREADID) {
    int2 uv = int2(p_dispatch_thread_id.xy);

    float4 clear = float4(0.0, 0.5, 0.5, 1.0);
    if (uv.x > 500 && uv.y > 500) {
        clear = float4(1.0, 0.4, 0.1, 1.0);
    }

    u_PathTracerOutputImage[uv] = clear;
}
