#pragma once
#include "component_manager.h"

namespace my::ecs {

// @TODO: combine View with reserved allocator and recycling
// so when removing a component, we don't need to remove it from the array,
// but we can invalidate it, and when creating View, View can skip it

using ViewContainer = std::vector<std::pair<Entity, int>>;

template<typename T>
class ViewIterator {
    using ComponentManagerType = typename std::conditional<std::is_const<T>::value,
                                                           const ComponentManager<typename std::remove_const<T>::type>,
                                                           ComponentManager<T>>::type;
    using Self = ViewIterator<T>;

public:
    ViewIterator(ViewContainer& p_container,
                 ComponentManagerType& p_manager,
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

    auto operator*() -> std::pair<Entity, T&> {
        Entity entity = m_container[m_index].first;
        int index = m_container[m_index].second;
        T& component = m_manager.GetComponentByIndex(index);
        return std::pair<Entity, T&>(entity, component);
    }

private:
    ViewContainer& m_container;
    ComponentManagerType& m_manager;

    size_t m_index{ 0 };
};

template<typename T>
class View {
    using ComponentManagerType = typename std::conditional<std::is_const<T>::value,
                                                           const ComponentManager<typename std::remove_const<T>::type>,
                                                           ComponentManager<T>>::type;

    using iter = ViewIterator<T>;

    View(ComponentManagerType& p_manager) : m_manager(p_manager), m_size(p_manager.GetCount()) {
        const int size = (int)p_manager.GetCount();
        for (int i = 0; i < size; ++i) {
            m_container.emplace_back(std::make_pair(m_manager.m_entityArray[i], i));
        }
    }

public:
    View(const View<T>&) = default;

    iter begin() { return iter(m_container, m_manager, 0); }
    iter end() { return iter(m_container, m_manager, m_size); }

private:
    ViewContainer m_container;
    ComponentManagerType& m_manager;
    size_t m_size;

    friend class Scene;
};

}  // namespace my::ecs
