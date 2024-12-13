#include "engine/scene/scene.h"

namespace my::ecs {

TEST(view, iterator) {
    Entity::SetSeed();
    Scene scene;
    for (int i = 0; i < 4; ++i) {
        scene.CreateNameEntity(std::format("entity_{}", i));
    }

    const char* names[] = {
        "entity_00",
        "entity_11",
        "entity_22",
        "entity_33",
    };

    {
        Scene& p_scene = scene;
        auto view = p_scene.View<NameComponent>();
        int i = 0;
        for (auto [id, name] : view) {
            name.GetNameRef().push_back(static_cast<char>('0' + i));
            ++i;
        }
    }

    {
        const Scene& p_scene = scene;
        auto view = p_scene.View<const NameComponent>();
        int i = 0;
        for (auto [id, name] : view) {
            EXPECT_EQ(name.GetName(), names[i]);
            ++i;
        }
    }
}

}  // namespace my::ecs
