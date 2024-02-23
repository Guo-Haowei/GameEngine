#include "rendering_misc.h"

#define DEFINE_DVAR
#include "rendering_dvars.h"

namespace my {

void register_rendering_dvars(){
#define REGISTER_DVAR
#include "rendering_dvars.h"
}

std::array<mat4, 6> cube_map_view_matrices(const vec3& p_eye) {
    std::array<mat4, 6> matrices;
    matrices[0] = glm::lookAt(p_eye, p_eye + glm::vec3(+1, +0, +0), glm::vec3(0, -1, +0));
    matrices[1] = glm::lookAt(p_eye, p_eye + glm::vec3(-1, +0, +0), glm::vec3(0, -1, +0));
    matrices[2] = glm::lookAt(p_eye, p_eye + glm::vec3(+0, +1, +0), glm::vec3(0, +0, +1));
    matrices[3] = glm::lookAt(p_eye, p_eye + glm::vec3(+0, -1, +0), glm::vec3(0, +0, -1));
    matrices[4] = glm::lookAt(p_eye, p_eye + glm::vec3(+0, +0, +1), glm::vec3(0, -1, +0));
    matrices[5] = glm::lookAt(p_eye, p_eye + glm::vec3(+0, +0, -1), glm::vec3(0, -1, +0));
    return matrices;
}

}  // namespace my