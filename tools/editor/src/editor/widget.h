#pragma once
#include "engine/math/geomath.h"
#include "engine/scene/scene.h"

namespace my {

constexpr float DEFAULT_COLUMN_WIDTH = 80.0f;

void PushDisabled();
void PopDisabled();

bool DrawDragInt(const char* p_label,
                 int& p_out,
                 float p_speed,
                 int p_min,
                 int p_max,
                 float p_column_width = DEFAULT_COLUMN_WIDTH);

bool DrawDragFloat(const char* p_label,
                   float& p_out,
                   float p_speed,
                   float p_min,
                   float p_max,
                   float p_column_width = DEFAULT_COLUMN_WIDTH);

bool DrawCheckBoxBitflag(const char* p_title, uint32_t& p_flags, const uint32_t p_bit);

bool DrawVec3Control(const char* p_label,
                     Vector3f& p_out,
                     float p_reset_value = 0.0f,
                     float p_column_width = DEFAULT_COLUMN_WIDTH);

bool DrawColorControl(const char* p_label,
                      Vector3f& p_out,
                      float p_reset_value = 1.0f,
                      float p_column_width = DEFAULT_COLUMN_WIDTH);

bool DrawInputText(const char* p_label,
                   std::string& p_string,
                   float p_column_width = DEFAULT_COLUMN_WIDTH);

bool DrawColorPicker3(const char* p_label,
                      float* p_out,
                      float p_column_width = DEFAULT_COLUMN_WIDTH);

}  // namespace my
