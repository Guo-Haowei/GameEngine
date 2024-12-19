#pragma once
#include "entity.h"

namespace YAML {
class Node;
class Emitter;
}  // namespace YAML

namespace my {
class Scene;
class Archive;
class FileAccess;

template<typename T>
concept Serializable = requires(T& t, Archive& p_archive, uint32_t p_version) {
    { t.Serialize(p_archive, p_version) } -> std::same_as<void>;
    { T::RegisterClass() } -> std::same_as<void>;
};

}  // namespace my

namespace my::ecs {

template<Serializable T>
class View;

// @TODO: remove this iterator, use view iterator instead
#define COMPONENT_MANAGER_ITERATOR_COMMON                                              \
public:                                                                                \
    self_type operator++(int) {                                                        \
        self_type tmp = *this;                                                         \
        ++m_index;                                                                     \
        return tmp;                                                                    \
    }                                                                                  \
    self_type operator--(int) {                                                        \
        self_type tmp = *this;                                                         \
        --m_index;                                                                     \
        return tmp;                                                                    \
    }                                                                                  \
    self_type& operator++() {                                                          \
        ++m_index;                                                                     \
        return *this;                                                                  \
    }                                                                                  \
    self_type& operator--() {                                                          \
        --m_index;                                                                     \
        return *this;                                                                  \
    }                                                                                  \
    bool operator==(const self_type& p_rhs) const { return m_index == p_rhs.m_index; } \
    bool operator!=(const self_type& p_rhs) const { return m_index != p_rhs.m_index; } \
    using _dummy_force_semi_colon = int

template<Serializable T>
class ComponentManagerIterator {
    using self_type = ComponentManagerIterator<T>;

public:
    ComponentManagerIterator(std::vector<Entity>& p_entity_array, std::vector<T>& p_component_array, size_t p_index)
        : m_entityArray(p_entity_array),
          m_componentArray(p_component_array),
          m_index(p_index) {}

    COMPONENT_MANAGER_ITERATOR_COMMON;

    std::pair<Entity, T&> operator*() const {
        return std::make_pair(this->m_entityArray[this->m_index], std::ref(this->m_componentArray[this->m_index]));
    }

private:
    std::vector<Entity>& m_entityArray;
    std::vector<T>& m_componentArray;

    size_t m_index;
};

template<Serializable T>
class ComponentManagerConstIterator {
    using self_type = ComponentManagerConstIterator<T>;

public:
    ComponentManagerConstIterator(const std::vector<Entity>& p_entity_array, const std::vector<T>& p_component_array, size_t p_index)
        : m_entityArray(p_entity_array),
          m_componentArray(p_component_array),
          m_index(p_index) {}

    COMPONENT_MANAGER_ITERATOR_COMMON;

    std::pair<Entity, const T&> operator*() const {
        return std::make_pair(this->m_entityArray[this->m_index], std::ref(this->m_componentArray[this->m_index]));
    }

private:
    const std::vector<Entity>& m_entityArray;
    const std::vector<T>& m_componentArray;

    size_t m_index;
};

class IComponentManager {
    IComponentManager(const IComponentManager&) = delete;
    IComponentManager& operator=(const IComponentManager&) = delete;

public:
    IComponentManager() = default;
    virtual ~IComponentManager() = default;
    virtual void Clear() = 0;
    virtual void Copy(const IComponentManager& p_other) = 0;
    virtual void Merge(IComponentManager& p_other) = 0;
    virtual void Remove(const Entity& p_entity) = 0;
    virtual bool Contains(const Entity& p_entity) const = 0;
    virtual size_t GetCount() const = 0;
    virtual Entity GetEntity(size_t p_index) const = 0;

    virtual const std::vector<Entity>& GetEntityArray() const = 0;

    virtual bool Serialize(Archive& p_archive, uint32_t p_version) = 0;
};

template<Serializable T>
class ComponentManager final : public IComponentManager {
    using iter = ComponentManagerIterator<T>;
    using const_iter = ComponentManagerConstIterator<T>;

public:
    iter begin() { return iter(m_entityArray, m_componentArray, 0); }
    iter end() { return iter(m_entityArray, m_componentArray, m_componentArray.size()); }
    const_iter begin() const { return const_iter(m_entityArray, m_componentArray, 0); }
    const_iter end() const { return const_iter(m_entityArray, m_componentArray, m_componentArray.size()); }

    ComponentManager(size_t p_capacity = 0) { Reserve(p_capacity); }

    void Reserve(size_t p_capacity);

    void Clear() override;

    void Copy(const ComponentManager<T>& p_other);

    void Copy(const IComponentManager& p_other) override;

    void Merge(ComponentManager<T>& p_other);

    void Merge(IComponentManager& p_other) override;

    void Remove(const Entity& p_entity) override;

    bool Contains(const Entity& p_entity) const override;

    T& GetComponentByIndex(size_t p_index);

    const T& GetComponentByIndex(size_t p_index) const;

    T* GetComponent(const Entity& p_entity);

    size_t GetCount() const override { return m_componentArray.size(); }

    Entity GetEntity(size_t p_index) const override;

    T& Create(const Entity& p_entity);

    const std::vector<Entity>& GetEntityArray() const override {
        return m_entityArray;
    }

    bool Serialize(Archive& p_archive, uint32_t p_version) override;

private:
    std::vector<T> m_componentArray;
    std::vector<Entity> m_entityArray;
    std::unordered_map<Entity, size_t> m_lookup;

    friend class ::my::Scene;
    friend class View<T>;
};

class ComponentLibrary {
public:
    struct LibraryEntry {
        std::unique_ptr<IComponentManager> m_manager = nullptr;
        uint64_t m_version = 0;
    };

    template<Serializable T>
    inline ComponentManager<T>& RegisterManager(const std::string& p_name, uint64_t p_version = 0) {
        DEV_ASSERT(m_entries.find(p_name) == m_entries.end());
        m_entries[p_name].m_manager = std::make_unique<ComponentManager<T>>();
        m_entries[p_name].m_version = p_version;
        return static_cast<ComponentManager<T>&>(*(m_entries[p_name].m_manager));
    }

private:
    std::unordered_map<std::string, LibraryEntry> m_entries;

    friend class ::my::Scene;
};

}  // namespace my::ecs
