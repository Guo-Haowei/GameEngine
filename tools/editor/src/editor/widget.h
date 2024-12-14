#pragma once
#include "engine/core/math/geomath.h"
#include "engine/scene/scene.h"

namespace my {

constexpr float DEFAULT_COLUMN_WIDTH = 90.0f;

void PushDisabled();
void PopDisabled();

bool DrawDragInt(const char* p_lable,
                 int& p_out,
                 float p_speed,
                 int p_min,
                 int p_max,
                 float p_column_width = DEFAULT_COLUMN_WIDTH);

bool DrawDragFloat(const char* p_lable,
                   float& p_out,
                   float p_speed,
                   float p_min,
                   float p_max,
                   float p_column_width = DEFAULT_COLUMN_WIDTH);

bool DrawVec3Control(const char* p_lable,
                     Vector3f& p_out,
                     float p_reset_value = 0.0f,
                     float p_column_width = DEFAULT_COLUMN_WIDTH);

bool DrawColorControl(const char* p_lable,
                      Vector3f& p_out,
                      float p_reset_value = 1.0f,
                      float p_column_width = DEFAULT_COLUMN_WIDTH);

}  // namespace my
