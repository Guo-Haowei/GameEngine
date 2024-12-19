#include "engine/systems/ecs/component_manager.h"

namespace my::ecs {

template<Serializable T>
struct dummy2 {
    using iter = ComponentManagerIterator<T>;
    using const_iter = ComponentManagerConstIterator<T>;

public:
    iter begin() { return iter(m_entityArray, m_componentArray, 0); }
    iter end() { return iter(m_entityArray, m_componentArray, m_componentArray.size()); }
    const_iter begin() const { return const_iter(m_entityArray, m_componentArray, 0); }
    const_iter end() const { return const_iter(m_entityArray, m_componentArray, m_componentArray.size()); }

    std::vector<Entity> m_entityArray;
    std::vector<T> m_componentArray;
};

template<Serializable T>
struct DummyTest {
};

struct A {
    int a;

    void Serialize(Archive&, uint32_t) {}
    bool Dump(YAML::Emitter&, FileAccess*, uint32_t) const {}
    bool Undump(const YAML::Node&, FileAccess*, uint32_t) {}
};

TEST(serializable, test_serializable) {
    struct B {};

    [[maybe_unused]] DummyTest<A> a;
#if 0
    // this won't compile
    [[maybe_unused]] DummyTest<B> b;
#endif
}

TEST(component_manager, iterator) {
    std::vector<Entity> entities = {
        Entity{ 1 }, Entity{ 2 },
        Entity{ 3 }, Entity{ 4 }
    };

    dummy2<A> aa;
    aa.m_componentArray = { {}, {}, {}, {} };
    aa.m_entityArray = entities;

    {
        int i = 0;
        for (const auto [entity, component] : aa) {
            EXPECT_EQ(&component, &aa.m_componentArray[i]);
            component.a = 10;
            EXPECT_EQ(entity, entities[i]);
            ++i;
        }
    }

    {
        const auto& p = aa;
        int i = 0;
        for (auto [entity, component] : p) {
            EXPECT_EQ(&component, &aa.m_componentArray[i++]);
        }
    }
}

}  // namespace my::ecs
