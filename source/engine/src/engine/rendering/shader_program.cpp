#include "shader_program.h"

#include "GLPrerequisites.h"

namespace my {

void ShaderProgram::bind() const {
    DEV_ASSERT(m_handle);
    glUseProgram(m_handle);
}

void ShaderProgram::unbind() const {
    glUseProgram(0);
}

}  // namespace my
