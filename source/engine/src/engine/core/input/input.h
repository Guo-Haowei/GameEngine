#pragma once
#include "core/math/geomath.h"
#include "input_code.h"

namespace my::input {

void beginFrame();
void endFrame();

bool isButtonDown(MouseButton p_key);
bool isButtonPressed(MouseButton p_key);
bool isButtonReleased(MouseButton p_key);

bool isKeyDown(KeyCode p_key);
bool isKeyPressed(KeyCode p_key);
bool isKeyReleased(KeyCode p_key);

const vec2& getCursor();
const vec2& getWheel();
vec2 mouseMove();

void setButton(int p_button, bool p_pressed);
void setKey(int p_key, bool p_pressed);

void setCursor(float p_x, float p_y);
void setWheel(float p_x, float p_y);

};  // namespace my::input
