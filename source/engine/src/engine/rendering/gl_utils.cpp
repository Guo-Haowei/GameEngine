#include "gl_utils.h"

#include "drivers/opengl/opengl_graphics_manager.h"
#include "drivers/opengl/opengl_prerequisites.h"

static OpenGLMeshBuffers g_quad;

void R_CreateQuad() {
    // clang-format off
    float points[] = {
        -1.0f, +1.0f,
        -1.0f, -1.0f,
        +1.0f, +1.0f,
        +1.0f, +1.0f,
        -1.0f, -1.0f,
        +1.0f, -1.0f,
    };
    // clang-format on
    glGenVertexArrays(1, &g_quad.vao);
    glGenBuffers(1, g_quad.vbos);
    glBindVertexArray(g_quad.vao);

    glBindBuffer(GL_ARRAY_BUFFER, g_quad.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
}

void R_DrawQuad() {
    DEV_ASSERT(g_quad.vao);
    glBindVertexArray(g_quad.vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

namespace gl {

uint64_t MakeTextureResident(uint32_t texture) {
    uint64_t ret = glGetTextureHandleARB(texture);
    glMakeTextureHandleResidentARB(ret);
    return ret;
}

}  // namespace gl