#pragma once
#include "core/math/geomath.h"
#include "scene/scene.h"

namespace my {

constexpr float DEFAULT_COLUMN_WIDTH = 90.0f;

void PushDisabled();
void PopDisabled();

bool DrawDragFloat(const char* p_lable,
                   float& p_out_float,
                   float p_speed,
                   float p_min,
                   float p_max,
                   float p_column_width = DEFAULT_COLUMN_WIDTH);

bool DrawVec3Control(const char* p_lable,
                     glm::vec3& p_out_vec3,
                     float p_reset_value = 0.0f,
                     float p_column_width = DEFAULT_COLUMN_WIDTH);

bool DrawColorControl(const char* p_lable,
                      glm::vec3& p_out_vec3,
                      float p_reset_value = 1.0f,
                      float p_column_width = DEFAULT_COLUMN_WIDTH);

}  // namespace my
