#include "core/systems/component_manager.h"

namespace my::ecs {

template<typename T>
struct dummy2 {
    using iter = ComponentManagerIterator<T>;
    using const_iter = ComponentManagerConstIterator<T>;

public:
    iter begin() { return iter(m_entity_array, m_component_array, 0); }
    iter end() { return iter(m_entity_array, m_component_array, m_component_array.size()); }
    const_iter begin() const { return const_iter(m_entity_array, m_component_array, 0); }
    const_iter end() const { return const_iter(m_entity_array, m_component_array, m_component_array.size()); }

    std::vector<Entity> m_entity_array;
    std::vector<T> m_component_array;
};

TEST(component_manager, iterator) {
    struct A {
        int a;
    };

    std::vector<Entity> entities = {
        Entity{ 1 }, Entity{ 2 },
        Entity{ 3 }, Entity{ 4 }
    };

    dummy2<A> aa;
    aa.m_component_array = { {}, {}, {}, {} };
    aa.m_entity_array = entities;

    {
        int i = 0;
        for (const auto [entity, component] : aa) {
            EXPECT_EQ(&component, &aa.m_component_array[i]);
            component.a = 10;
            EXPECT_EQ(entity, entities[i]);
            ++i;
        }
    }

    {
        const auto& p = aa;
        int i = 0;
        for (auto [entity, component] : p) {
            EXPECT_EQ(&component, &aa.m_component_array[i++]);
        }
    }
}

}  // namespace my::ecs
