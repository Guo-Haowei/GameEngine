#pragma once
#include "component_manager.h"

namespace my::ecs {

// @TODO: combine View with reserved allocator and recycling
// so when removing a component, we don't need to remove it from the array,
// but we can invalidate it, and when creating View, View can skip it

template<Serializable T>
class View {
    using ViewContainer = std::vector<std::pair<Entity, int>>;

public:
    template<typename U>
        requires std::is_same_v<T, std::remove_const_t<U>>
    class Iterator;

    using iter = Iterator<T>;
    using const_iter = Iterator<const T>;

#pragma region ITERATOR
    template<typename U>
        requires std::is_same_v<T, std::remove_const_t<U>>
    class Iterator {
        using Self = Iterator<U>;

    public:
        Iterator(const ViewContainer& p_container,
                 const ComponentManager<T>& p_manager,
                 size_t p_index) : m_container(p_container),
                                   m_manager(p_manager),
                                   m_index(p_index) {}

        Self operator++(int) const {
            Self tmp = *this;
            ++m_index;
            return tmp;
        }

        Self operator--(int) const {
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

        auto operator*() -> std::pair<Entity, U&> {
            Entity entity = m_container[m_index].first;
            int index = m_container[m_index].second;
            U& component = ((ComponentManager<T>&)m_manager).GetComponentByIndex(index);
            return std::pair<Entity, U&>(entity, component);
        }

        auto operator*() const -> std::pair<Entity, U&> {
            Entity entity = m_container[m_index].first;
            int index = m_container[m_index].second;
            U& component = m_manager.GetComponentByIndex(index);
            return std::pair<Entity, U&>(entity, component);
        }

    private:
        const ViewContainer& m_container;
        const ComponentManager<T>& m_manager;

        size_t m_index{ 0 };
    };
#pragma endregion ITERATOR

    View(const ComponentManager<T>& p_manager) : m_manager(p_manager), m_size(p_manager.GetCount()) {
        const int size = (int)p_manager.GetCount();
        for (int i = 0; i < size; ++i) {
            m_container.emplace_back(std::make_pair(m_manager.m_entityArray[i], i));
        }
    }

    View(const View<T>&) = default;

    iter begin() { return iter(m_container, m_manager, 0); }
    iter end() { return iter(m_container, m_manager, m_size); }

    const_iter begin() const { return const_iter(m_container, m_manager, 0); }
    const_iter end() const { return const_iter(m_container, m_manager, m_size); }

private:
    ViewContainer m_container;
    const ComponentManager<T>& m_manager;
    size_t m_size;
};

}  // namespace my::ecs
