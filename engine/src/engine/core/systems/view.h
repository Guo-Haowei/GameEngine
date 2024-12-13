#pragma once
#include "component_manager.h"

namespace my::ecs {

// @TODO: combine View with reserved allocator and recycling
// so when removing a component, we don't need to remove it from the array,
// but we can invalidate it, and when creating View, View can skip it

using ViewContainer = std::vector<std::pair<Entity, int>>;

template<typename T, typename U>
class ViewIterator {
    using Self = ViewIterator<T, U>;

public:
    ViewIterator(ViewContainer& p_container,
                 ecs::ComponentManager<T>& p_manager,
                 size_t p_index) : m_container(p_container),
                                   m_manager(p_manager),
                                   m_index(p_index) {}

    Self operator++(int) {
        Self tmp = *this;
        ++m_index;
        return tmp;
    }

    Self operator--(int) {
        Self tmp = *this;
        --m_index;
        return tmp;
    }

    Self& operator++() {
        ++m_index;
        return *this;
    }

    Self& operator--() {
        --m_index;
        return *this;
    }

    bool operator==(const Self& p_rhs) const { return m_index == p_rhs.m_index; }
    bool operator!=(const Self& p_rhs) const { return m_index != p_rhs.m_index; }

    auto operator*() -> std::pair<Entity, U&> const {
        Entity entity = m_container[m_index].first;
        int index = m_container[m_index].second;
        U& component = m_manager.GetComponent(index);
        return std::pair<Entity, U&>(entity, component);
    }

private:
    ViewContainer& m_container;
    ecs::ComponentManager<T>& m_manager;

    size_t m_index{ 0 };
};

template<typename T>
class View {
    using iter = ViewIterator<T, T>;
    using const_iter = ViewIterator<T, const T>;

    View(ecs::ComponentManager<T>& p_manager) : m_manager(p_manager), m_size(p_manager.GetCount()) {
        const int size = (int)p_manager.GetCount();
        for (int i = 0; i < size; ++i) {
            m_container.emplace_back(std::make_pair(m_manager.m_entityArray[i], i));
        }
    }

public:
    View(const View<T>&) = default;

    iter begin() { return iter(m_container, m_manager, 0); }
    iter end() { return iter(m_container, m_manager, m_size); }
    const_iter begin() const { return const_iter(m_container, m_manager, 0); }
    const_iter end() const { return const_iter(m_container, m_manager, m_size); }

private:
    ViewContainer m_container;
    ecs::ComponentManager<T>& m_manager;
    size_t m_size;

    friend class Scene;
};

}  // namespace my::ecs
