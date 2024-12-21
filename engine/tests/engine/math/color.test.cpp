#include "engine/math/color.h"

namespace my::math {

TEST(color, to_rgb) {
    auto c = Color::Hex(ColorCode::COLOR_RED);
    uint32_t rgb = c.ToRgb();
    EXPECT_EQ(rgb, ColorCode::COLOR_RED);
}

}  // namespace my::math
