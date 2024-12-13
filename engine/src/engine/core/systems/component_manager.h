#pragma once
#include "engine/core/io/archive.h"
#include "entity.h"

namespace my {
class Scene;
}

namespace my::ecs {

template<typename T>
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

template<typename T>
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

template<typename T>
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

    virtual bool Serialize(Archive& p_archive, uint32_t p_version) = 0;
};

template<typename T>
class ComponentManager final : public IComponentManager {
    using iter = ComponentManagerIterator<T>;
    using const_iter = ComponentManagerConstIterator<T>;

public:
    iter begin() { return iter(m_entityArray, m_componentArray, 0); }
    iter end() { return iter(m_entityArray, m_componentArray, m_componentArray.size()); }
    const_iter begin() const { return const_iter(m_entityArray, m_componentArray, 0); }
    const_iter end() const { return const_iter(m_entityArray, m_componentArray, m_componentArray.size()); }

    ComponentManager(size_t p_capacity = 0) { Reserve(p_capacity); }

    void Reserve(size_t p_capacity) {
        if (p_capacity) {
            m_componentArray.reserve(p_capacity);
            m_entityArray.reserve(p_capacity);
            m_lookup.reserve(p_capacity);
        }
    }

    void Clear() override {
        m_componentArray.clear();
        m_entityArray.clear();
        m_lookup.clear();
    }

    void Copy(const ComponentManager<T>& p_other) {
        Clear();
        m_componentArray = p_other.m_componentArray;
        m_entityArray = p_other.m_entityArray;
        m_lookup = p_other.m_lookup;
    }

    void Copy(const IComponentManager& p_other) override {
        Copy((ComponentManager<T>&)p_other);
    }

    void Merge(ComponentManager<T>& p_other) {
        const size_t reserved = GetCount() + p_other.GetCount();
        m_componentArray.reserve(reserved);
        m_entityArray.reserve(reserved);
        m_lookup.reserve(reserved);

        for (size_t i = 0; i < p_other.GetCount(); ++i) {
            Entity entity = p_other.m_entityArray[i];
            DEV_ASSERT(!Contains(entity));
            m_entityArray.push_back(entity);
            m_lookup[entity] = m_componentArray.size();
            m_componentArray.push_back(std::move(p_other.m_componentArray[i]));
        }

        p_other.Clear();
    }

    void Merge(IComponentManager& p_other) override {
        Merge((ComponentManager<T>&)p_other);
    }

    void Remove(const Entity& p_entity) override {
        auto it = m_lookup.find(p_entity);
        if (it == m_lookup.end()) {
            return;
        }

        size_t index = it->second;
        DEV_ASSERT_INDEX(index, m_entityArray.size());
        m_lookup.erase(it);
        m_entityArray.erase(m_entityArray.begin() + index);
        m_componentArray.erase(m_componentArray.begin() + index);
        for (auto& iter : m_lookup) {
            DEV_ASSERT(iter.second != index);
            if (iter.second > index) {
                --iter.second;
            }
        }
    }

    bool Contains(const Entity& p_entity) const override {
        if (m_lookup.empty()) {
            return false;
        }
        return m_lookup.find(p_entity) != m_lookup.end();
    }

    inline T& GetComponentByIndex(size_t p_index) {
        DEV_ASSERT(p_index < m_componentArray.size());
        return m_componentArray[p_index];
    }

    inline const T& GetComponentByIndex(size_t p_index) const {
        DEV_ASSERT(p_index < m_componentArray.size());
        return m_componentArray[p_index];
    }

    T* GetComponent(const Entity& p_entity) {
        if (!p_entity.IsValid() || m_lookup.empty()) {
            return nullptr;
        }

        auto it = m_lookup.find(p_entity);

        if (it == m_lookup.end()) {
            return nullptr;
        }

        return &m_componentArray[it->second];
    }

    size_t GetCount() const override { return m_componentArray.size(); }

    Entity GetEntity(size_t p_index) const override {
        DEV_ASSERT(p_index < m_entityArray.size());
        return m_entityArray[p_index];
    }

    T& Create(const Entity& p_entity) {
        DEV_ASSERT(p_entity.IsValid());

        const size_t componentCount = m_componentArray.size();
        DEV_ASSERT(m_lookup.find(p_entity) == m_lookup.end());
        DEV_ASSERT(m_entityArray.size() == componentCount);
        DEV_ASSERT(m_lookup.size() == componentCount);

        m_lookup[p_entity] = componentCount;
        m_componentArray.emplace_back();
        m_entityArray.push_back(p_entity);
        return m_componentArray.back();
    }

    bool Serialize(Archive& p_archive, uint32_t p_version) override {
        constexpr uint64_t magic = 7165065861825654388llu;
        size_t count;
        if (p_archive.IsWriteMode()) {
            p_archive << magic;
            count = static_cast<uint32_t>(m_componentArray.size());
            p_archive << count;
            for (auto& component : m_componentArray) {
                component.Serialize(p_archive, p_version);
            }
            for (auto& entity : m_entityArray) {
                entity.Serialize(p_archive);
            }
        } else {
            uint64_t read_magic;
            p_archive >> read_magic;
            if (read_magic != magic) {
                return false;
            }

            Clear();
            p_archive >> count;
            m_componentArray.resize(count);
            m_entityArray.resize(count);
            for (size_t i = 0; i < count; ++i) {
                m_componentArray[i].Serialize(p_archive, p_version);
            }
            for (size_t i = 0; i < count; ++i) {
                m_entityArray[i].Serialize(p_archive);
                m_lookup[m_entityArray[i]] = i;
            }
        }

        return true;
    }

    // @TODO: change it to private
public:
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

    template<typename T>
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
