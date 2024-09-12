#include "input.h"

#include <bitset>

namespace my::input {

static struct {
    std::bitset<KEY_MAX> keys;
    std::bitset<KEY_MAX> prev_keys;
    std::bitset<MOUSE_BUTTON_MAX> buttons = { false };
    std::bitset<MOUSE_BUTTON_MAX> prev_buttons = { false };

    vec2 cursor{ 0, 0 };
    vec2 prev_cursor{ 0, 0 };

    vec2 wheel{ 0, 0 };
} s_glob;

void beginFrame() {
}

void endFrame() {
    s_glob.prev_keys = s_glob.keys;
    s_glob.prev_buttons = s_glob.buttons;
    s_glob.prev_cursor = s_glob.cursor;
    s_glob.wheel = vec2(0);
}

template<size_t N>
static inline bool inputIsDown(const std::bitset<N>& p_array, int p_index) {
    DEV_ASSERT_INDEX(p_index, N);
    return p_array[p_index];
}

template<size_t N>
static inline bool inputHasChanged(const std::bitset<N>& p_current, const std::bitset<N>& p_prev, int p_index) {
    DEV_ASSERT_INDEX(p_index, N);
    return p_current[p_index] == true && p_prev[p_index] == false;
}

bool isButtonDown(MouseButton p_key) {
    return inputIsDown(s_glob.buttons, std::to_underlying(p_key));
}

bool isButtonPressed(MouseButton p_key) {
    return inputHasChanged(s_glob.buttons, s_glob.prev_buttons, std::to_underlying(p_key));
}

bool isButtonReleased(MouseButton p_key) {
    return inputHasChanged(s_glob.prev_buttons, s_glob.buttons, std::to_underlying(p_key));
}

bool isKeyDown(KeyCode p_key) {
    return inputIsDown(s_glob.keys, std::to_underlying(p_key));
}

bool isKeyPressed(KeyCode p_key) {
    return inputHasChanged(s_glob.keys, s_glob.prev_keys, std::to_underlying(p_key));
}

bool isKeyReleased(KeyCode p_key) {
    return inputHasChanged(s_glob.prev_keys, s_glob.keys, std::to_underlying(p_key));
}

vec2 mouseMove() {
    vec2 point;
    point = s_glob.cursor - s_glob.prev_cursor;
    return point;
}

const vec2& getCursor() { return s_glob.cursor; }

const vec2& getWheel() { return s_glob.wheel; }

void setButton(int p_button, bool p_pressed) {
    ERR_FAIL_INDEX(p_button, MOUSE_BUTTON_MAX);
    s_glob.buttons[p_button] = p_pressed;
}

void setKey(int p_key, bool p_pressed) {
    ERR_FAIL_INDEX(p_key, KEY_MAX);
    s_glob.keys[p_key] = p_pressed;
}

void setCursor(float p_x, float p_y) {
    s_glob.cursor.x = p_x;
    s_glob.cursor.y = p_y;
}

void setWheel(float p_x, float p_y) {
    s_glob.wheel.x = p_x;
    s_glob.wheel.y = p_y;
}
}  // namespace my::input
