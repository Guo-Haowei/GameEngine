#include "engine/scene/scene.h"

namespace my::ecs {

TEST(View, iterator) {

    Scene scene;
    for (int i = 0; i < 4; ++i) {
        scene.CreateNameEntity(std::format("entity_{}", i));
    }

    auto view = scene.View<NameComponent>();

    {
        const char* names[] = {
            "entity_00",
            "entity_11",
            "entity_22",
            "entity_33",
        };

        int i = 0;
        for (const auto [id, name] : view) {
            name.GetNameRef().push_back('0' + i);
            ++i;
        }

        i = 0;
        for (const auto [id, name] : view) {
            EXPECT_EQ(name.GetName(), names[i++]);
        }
    }
}

}  // namespace my::ecs
