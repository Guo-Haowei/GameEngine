#pragma once
#include "core/io/archive.h"
#include "entity.h"

namespace my {
class Scene;
}

namespace my::ecs {

#define COMPONENT_MANAGER_ITERATOR_COMMON                                          \
public:                                                                            \
    self_type operator++(int) {                                                    \
        self_type tmp = *this;                                                     \
        ++m_index;                                                                 \
        return tmp;                                                                \
    }                                                                              \
    self_type operator--(int) {                                                    \
        self_type tmp = *this;                                                     \
        --m_index;                                                                 \
        return tmp;                                                                \
    }                                                                              \
    self_type& operator++() {                                                      \
        ++m_index;                                                                 \
        return *this;                                                              \
    }                                                                              \
    self_type& operator--() {                                                      \
        --m_index;                                                                 \
        return *this;                                                              \
    }                                                                              \
    bool operator==(const self_type& rhs) const { return m_index == rhs.m_index; } \
    bool operator!=(const self_type& rhs) const { return m_index != rhs.m_index; } \
    using _dummy_force_semi_colon = int

template<typename T>
class ComponentManagerIterator {
    using self_type = ComponentManagerIterator<T>;

public:
    ComponentManagerIterator(std::vector<Entity>& p_entity_array, std::vector<T>& p_component_array, size_t p_index)
        : m_entity_array(p_entity_array),
          m_component_array(p_component_array),
          m_index(p_index) {}

    COMPONENT_MANAGER_ITERATOR_COMMON;

    std::pair<Entity, T&> operator*() const {
        return std::make_pair(this->m_entity_array[this->m_index], std::ref(this->m_component_array[this->m_index]));
    }

private:
    std::vector<Entity>& m_entity_array;
    std::vector<T>& m_component_array;

    size_t m_index;
};

template<typename T>
class ComponentManagerConstIterator {
    using self_type = ComponentManagerConstIterator<T>;

public:
    ComponentManagerConstIterator(const std::vector<Entity>& p_entity_array, const std::vector<T>& p_component_array, size_t p_index)
        : m_entity_array(p_entity_array),
          m_component_array(p_component_array),
          m_index(p_index) {}

    COMPONENT_MANAGER_ITERATOR_COMMON;

    std::pair<Entity, const T&> operator*() const {
        return std::make_pair(this->m_entity_array[this->m_index], std::ref(this->m_component_array[this->m_index]));
    }

private:
    const std::vector<Entity>& m_entity_array;
    const std::vector<T>& m_component_array;

    size_t m_index;
};

class IComponentManager {
    IComponentManager(const IComponentManager&) = delete;
    IComponentManager& operator=(const IComponentManager&) = delete;

public:
    IComponentManager() = default;
    virtual ~IComponentManager() = default;
    virtual void clear() = 0;
    virtual void copy(const IComponentManager& other) = 0;
    virtual void merge(IComponentManager& other) = 0;
    virtual void remove(const Entity& entity) = 0;
    virtual bool contains(const Entity& entity) const = 0;
    virtual size_t get_index(const Entity& entity) const = 0;
    virtual size_t get_count() const = 0;
    virtual Entity get_entity(size_t index) const = 0;
};

template<typename T>
class ComponentManager final : public IComponentManager {
    using iter = ComponentManagerIterator<T>;
    using const_iter = ComponentManagerConstIterator<T>;

public:
    iter begin() { return iter(m_entity_array, m_component_array, 0); }
    iter end() { return iter(m_entity_array, m_component_array, m_component_array.size()); }
    const_iter begin() const { return const_iter(m_entity_array, m_component_array, 0); }
    const_iter end() const { return const_iter(m_entity_array, m_component_array, m_component_array.size()); }

    ComponentManager(size_t capacity = 0) { reserve(capacity); }

    void reserve(size_t capacity) {
        if (capacity) {
            m_component_array.reserve(capacity);
            m_entity_array.reserve(capacity);
            m_lookup.reserve(capacity);
        }
    }

    void clear() override {
        m_component_array.clear();
        m_entity_array.clear();
        m_lookup.clear();
    }

    void copy(const ComponentManager<T>& other) {
        clear();
        m_component_array = other.m_component_array;
        m_entity_array = other.m_entity_array;
        m_lookup = other.m_lookup;
    }

    void copy(const IComponentManager& other) override {
        copy((ComponentManager<T>&)other);
    }

    void merge(ComponentManager<T>& other) {
        const size_t reserved = get_count() + other.get_count();
        m_component_array.reserve(reserved);
        m_entity_array.reserve(reserved);
        m_lookup.reserve(reserved);

        for (size_t i = 0; i < other.get_count(); ++i) {
            Entity entity = other.m_entity_array[i];
            DEV_ASSERT(!contains(entity));
            m_entity_array.push_back(entity);
            m_lookup[entity] = m_component_array.size();
            m_component_array.push_back(std::move(other.m_component_array[i]));
        }

        other.clear();
    }

    void merge(IComponentManager& other) override {
        merge((ComponentManager<T>&)other);
    }

    void remove(const Entity& entity) override {
        auto it = m_lookup.find(entity);
        if (it == m_lookup.end()) {
            return;
        }

        size_t index = it->second;
        m_lookup.erase(it);
        DEV_ASSERT_INDEX(index, m_entity_array.size());
        m_entity_array[index] = Entity::INVALID;
    }

    bool contains(const Entity& entity) const override {
        if (m_lookup.empty()) {
            return false;
        }
        return m_lookup.find(entity) != m_lookup.end();
    }

    inline T& get_component(size_t idx) {
        DEV_ASSERT(idx < m_component_array.size());
        return m_component_array[idx];
    }

    T* get_component(const Entity& entity) {
        if (!entity.is_valid() || m_lookup.empty()) {
            return nullptr;
        }

        auto it = m_lookup.find(entity);

        if (it == m_lookup.end()) {
            return nullptr;
        }

        return &m_component_array[it->second];
    }

    size_t get_index(const Entity& entity) const override {
        if (m_lookup.empty()) {
            return Entity::INVALID_INDEX;
        }

        const auto it = m_lookup.find(entity);
        if (it == m_lookup.end()) {
            return Entity::INVALID_INDEX;
        }

        return it->second;
    }

    size_t get_count() const override { return m_component_array.size(); }

    Entity get_entity(size_t index) const override {
        DEV_ASSERT(index < m_entity_array.size());
        return m_entity_array[index];
    }

    T& create(const Entity& entity) {
        DEV_ASSERT(entity.is_valid());

        const size_t componentCount = m_component_array.size();
        DEV_ASSERT(m_lookup.find(entity) == m_lookup.end());
        DEV_ASSERT(m_entity_array.size() == componentCount);
        DEV_ASSERT(m_lookup.size() == componentCount);

        m_lookup[entity] = componentCount;
        m_component_array.emplace_back();
        m_entity_array.push_back(entity);
        return m_component_array.back();
    }

    const std::vector<T>& get_component_array() const { return m_component_array; }
    std::vector<T>& get_component_array() { return m_component_array; }

    const T& operator[](size_t idx) const { return get_component(idx); }

    T& operator[](size_t idx) { return get_component(idx); }

    bool serialize(Archive& archive, uint32_t version) {
        constexpr uint64_t magic = 7165065861825654388llu;
        size_t count;
        if (archive.is_write_mode()) {
            archive << magic;
            count = static_cast<uint32_t>(m_component_array.size());
            archive << count;
            for (auto& component : m_component_array) {
                component.serialize(archive, version);
            }
            for (auto& entity : m_entity_array) {
                entity.serialize(archive);
            }
        } else {
            uint64_t read_magic;
            archive >> read_magic;
            if (read_magic != magic) {
                return false;
            }

            clear();
            archive >> count;
            m_component_array.resize(count);
            m_entity_array.resize(count);
            for (size_t i = 0; i < count; ++i) {
                m_component_array[i].serialize(archive, version);
            }
            for (size_t i = 0; i < count; ++i) {
                m_entity_array[i].serialize(archive);
                m_lookup[m_entity_array[i]] = i;
            }
        }

        return true;
    }

private:
    std::vector<T> m_component_array;
    std::vector<Entity> m_entity_array;
    std::unordered_map<Entity, size_t> m_lookup;
};

class ComponentLibrary {
public:
    struct LibraryEntry {
        std::unique_ptr<IComponentManager> m_manager = nullptr;
        uint64_t m_version = 0;
    };

    template<typename T>
    inline ComponentManager<T>& register_manager(const std::string& name, uint64_t version = 0) {
        DEV_ASSERT(m_entries.find(name) == m_entries.end());
        m_entries[name].m_manager = std::make_unique<ComponentManager<T>>();
        m_entries[name].m_version = version;
        return static_cast<ComponentManager<T>&>(*(m_entries[name].m_manager));
    }

private:
    std::unordered_map<std::string, LibraryEntry> m_entries;

    friend class Scene;
};

}  // namespace my::ecs
