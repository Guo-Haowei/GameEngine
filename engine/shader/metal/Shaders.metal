#include <metal_stdlib>
using namespace metal;

// xcrun -sdk macosx metal -c Shaders.metal -o Shaders.air
// xcrun -sdk macosx metallib Shaders.air -o Shaders.metallib

// Vertex shader
vertex float4 vertex_main(const device float3 *vertexArray [[ buffer(0) ]],
                          uint vertexID [[ vertex_id ]]) {
    return float4(vertexArray[vertexID], 1.0);
}

// Fragment shader
fragment float4 fragment_main() {
    return float4(1.0, 0.0, 0.0, 1.0); // Red color
}