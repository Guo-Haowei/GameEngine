#include "input.h"

#include <bitset>

namespace my::input {

static struct {
    std::bitset<KEY_MAX> keys;
    std::bitset<KEY_MAX> prevKeys;
    std::bitset<MOUSE_BUTTON_MAX> buttons = { false };
    std::bitset<MOUSE_BUTTON_MAX> prevButtons = { false };

    vec2 cursor{ 0, 0 };
    vec2 prevCursor{ 0, 0 };

    vec2 wheel{ 0, 0 };
} s_glob;

void BeginFrame() {
}

void EndFrame() {
    s_glob.prevKeys = s_glob.keys;
    s_glob.prevButtons = s_glob.buttons;
    s_glob.prevCursor = s_glob.cursor;
    s_glob.wheel = vec2(0);
}

template<size_t N>
static inline bool InputIsDown(const std::bitset<N>& p_array, int p_index) {
    DEV_ASSERT_INDEX(p_index, N);
    return p_array[p_index];
}

template<size_t N>
static inline bool InputHasChanged(const std::bitset<N>& p_current, const std::bitset<N>& p_prev, int p_index) {
    DEV_ASSERT_INDEX(p_index, N);
    return p_current[p_index] == true && p_prev[p_index] == false;
}

bool IsButtonDown(MouseButton p_key) {
    return InputIsDown(s_glob.buttons, std::to_underlying(p_key));
}

bool IsButtonPressed(MouseButton p_key) {
    return InputHasChanged(s_glob.buttons, s_glob.prevButtons, std::to_underlying(p_key));
}

bool IsButtonReleased(MouseButton p_key) {
    return InputHasChanged(s_glob.prevButtons, s_glob.buttons, std::to_underlying(p_key));
}

bool IsKeyDown(KeyCode p_key) {
    return InputIsDown(s_glob.keys, std::to_underlying(p_key));
}

bool IsKeyPressed(KeyCode p_key) {
    return InputHasChanged(s_glob.keys, s_glob.prevKeys, std::to_underlying(p_key));
}

bool IsKeyReleased(KeyCode p_key) {
    return InputHasChanged(s_glob.prevKeys, s_glob.keys, std::to_underlying(p_key));
}

vec2 MouseMove() {
    vec2 point;
    point = s_glob.cursor - s_glob.prevCursor;
    return point;
}

const vec2& GetCursor() { return s_glob.cursor; }

const vec2& GetWheel() { return s_glob.wheel; }

void SetButton(int p_button, bool p_pressed) {
    ERR_FAIL_INDEX(p_button, MOUSE_BUTTON_MAX);
    s_glob.buttons[p_button] = p_pressed;
}

void SetKey(int p_key, bool p_pressed) {
    ERR_FAIL_INDEX(p_key, KEY_MAX);
    s_glob.keys[p_key] = p_pressed;
}

void SetCursor(float p_x, float p_y) {
    s_glob.cursor.x = p_x;
    s_glob.cursor.y = p_y;
}

void SetWheel(float p_x, float p_y) {
    s_glob.wheel.x = p_x;
    s_glob.wheel.y = p_y;
}
}  // namespace my::input
