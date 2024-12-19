#include "engine/scene/scene.h"

namespace my::ecs {

template<typename T>
static inline void CheckConst(T&&) {
    static_assert(std::is_const_v<std::remove_reference_t<T>>);
}

TEST(view, iterator) {
    Entity::SetSeed();
    Scene scene;
    const Scene& const_scene = scene;
    for (int i = 0; i < 4; ++i) {
        scene.CreateNameEntity(std::format("entity_{}", i));
    }

    const char* names[] = {
        "entity_00",
        "entity_11",
        "entity_22",
        "entity_33",
    };

    int i = 0;
    for (auto [id, name] : scene.View<NameComponent>()) {
        name.GetNameRef().push_back(static_cast<char>('0' + i));
        ++i;
    }

    i = 0;
    for (auto [id, name] : const_scene.View<NameComponent>()) {
        CheckConst(name);
        EXPECT_EQ(name.GetName(), names[i]);
        ++i;
    }
}

}  // namespace my::ecs
