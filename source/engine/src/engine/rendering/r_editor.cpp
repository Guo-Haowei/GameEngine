#include "r_editor.h"

#include "GpuTexture.h"
#include "core/framework/scene_manager.h"
#include "core/math/geometry.h"
#include "gl_utils.h"
#include "r_cbuffers.h"

using namespace my;

struct VertexPoint3D {
    vec3 position;
    vec3 color;
};

struct TextureVertex {
    vec2 pos;
    vec2 uv;
};

// @TODO: fix
static inline void FillTextureIconBuffer(std::vector<TextureVertex>& iconBuffer, const vec2& offset, float aspect) {
    constexpr TextureVertex kVertices[] = {
        { vec2(-1, +1), vec2(0, 0) },  // top-left
        { vec2(-1, -1), vec2(0, 1) },  // bottom-left
        { vec2(+1, -1), vec2(1, 1) },  // bottom-right
        { vec2(+1, +1), vec2(1, 0) },  // top-right
    };

    constexpr uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };
    constexpr float kScale = 0.07f;
    const vec2 scale(kScale, aspect * kScale);

    for (size_t idx = 0; idx < my::array_length(indices); ++idx) {
        TextureVertex vertex = kVertices[indices[idx]];
        vertex.pos *= scale;
        vertex.pos += offset;
        iconBuffer.emplace_back(vertex);
    }
}
