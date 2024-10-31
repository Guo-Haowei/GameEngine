#pragma once
#include "core/math/geomath.h"
#include "input_code.h"

namespace my::input {

void BeginFrame();
void EndFrame();

bool IsButtonDown(MouseButton p_key);
bool IsButtonPressed(MouseButton p_key);
bool IsButtonReleased(MouseButton p_key);

bool IsKeyDown(KeyCode p_key);
bool IsKeyPressed(KeyCode p_key);
bool IsKeyReleased(KeyCode p_key);

const vec2& GetCursor();
const vec2& GetWheel();
vec2 MouseMove();

void SetButton(int p_button, bool p_pressed);
void SetKey(int p_key, bool p_pressed);

void SetCursor(float p_x, float p_y);
void SetWheel(float p_x, float p_y);

};  // namespace my::input
