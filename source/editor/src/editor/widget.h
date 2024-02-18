#pragma once
#include "core/math/geomath.h"
#include "scene/scene.h"

namespace my {

constexpr float kDefaultColumnWidth = 90.0f;

void push_disabled();
void pop_disabled();

bool draw_drag_float(const char* p_lable,
                     float& p_out_float,
                     float p_speed,
                     float p_min,
                     float p_max,
                     float p_column_width = kDefaultColumnWidth);

bool draw_vec3_control(const char* p_lable,
                       glm::vec3& p_out_vec3,
                       float p_reset_value = 0.0f,
                       float p_column_width = kDefaultColumnWidth);

bool draw_color_control(const char* p_lable,
                        glm::vec3& p_out_vec3,
                        float p_reset_value = 1.0f,
                        float p_column_width = kDefaultColumnWidth);

void scene_add_dropdown(Scene& scene);

}  // namespace my
