/// File: common.hlsl.h
#if defined(HLSL_LANG)
#define MUL(a, b) mul((a), (b))
#elif defined(GLSL_LANG)
#define MUL(a, b) ((a) * (b))
#endif

// reconstruct view position
// https://stackoverflow.com/questions/11277501/how-to-recover-view-space-position-given-view-space-depth-value-and-ndc-xy
Vector3f NdcToViewPos(Vector2f uv, float depth) {
    Vector2f ndc = 2.0f * uv - 1.0f;
#if defined(GLSL_LANG)
    Vector4f screen_pos = Vector4f(ndc.x, ndc.y, 2.0f * depth - 1.0f, 1.0f);
#else
    Vector4f screen_pos = Vector4f(ndc.x, ndc.y, depth, 1.0f);
#endif
    Vector4f viewPosH = MUL(c_invCamProj, screen_pos);
    Vector3f viewPos = viewPosH.xyz / viewPosH.w;
    return viewPos;
}
