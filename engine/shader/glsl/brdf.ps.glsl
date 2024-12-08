/// File: brdf.ps.glsl
layout(location = 0) in vec2 pass_uv;
layout(location = 0) out vec2 out_color;

#include "../pbr.hlsl.h"

void main() {
    vec2 integratedBRDF = IntegrateBRDF(pass_uv.x, pass_uv.y);
    out_color = integratedBRDF;
}
