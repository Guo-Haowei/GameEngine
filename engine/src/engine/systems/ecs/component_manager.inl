#pragma once
#include <yaml-cpp/yaml.h>

#include "component_manager.h"

namespace my::ecs {

template<Serializable T>
void ComponentManager<T>::Reserve(size_t p_capacity) {
    if (p_capacity) {
        m_componentArray.reserve(p_capacity);
        m_entityArray.reserve(p_capacity);
        m_lookup.reserve(p_capacity);
    }
}

template<Serializable T>
void ComponentManager<T>::Clear() {
    m_componentArray.clear();
    m_entityArray.clear();
    m_lookup.clear();
}

template<Serializable T>
void ComponentManager<T>::Copy(const ComponentManager<T>& p_other) {
    Clear();
    m_componentArray = p_other.m_componentArray;
    m_entityArray = p_other.m_entityArray;
    m_lookup = p_other.m_lookup;
}

template<Serializable T>
void ComponentManager<T>::Copy(const IComponentManager& p_other) {
    Copy((ComponentManager<T>&)p_other);
}

template<Serializable T>
void ComponentManager<T>::Merge(ComponentManager<T>& p_other) {
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

template<Serializable T>
void ComponentManager<T>::Merge(IComponentManager& p_other) {
    Merge((ComponentManager<T>&)p_other);
}

template<Serializable T>
void ComponentManager<T>::Remove(const Entity& p_entity) {
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

template<Serializable T>
bool ComponentManager<T>::Contains(const Entity& p_entity) const {
    if (m_lookup.empty()) {
        return false;
    }
    return m_lookup.find(p_entity) != m_lookup.end();
}

template<Serializable T>
T& ComponentManager<T>::GetComponentByIndex(size_t p_index) {
    DEV_ASSERT(p_index < m_componentArray.size());
    return m_componentArray[p_index];
}

template<Serializable T>
const T& ComponentManager<T>::GetComponentByIndex(size_t p_index) const {
    DEV_ASSERT(p_index < m_componentArray.size());
    return m_componentArray[p_index];
}

template<Serializable T>
T* ComponentManager<T>::GetComponent(const Entity& p_entity) {
    if (!p_entity.IsValid() || m_lookup.empty()) {
        return nullptr;
    }

    auto it = m_lookup.find(p_entity);

    if (it == m_lookup.end()) {
        return nullptr;
    }

    return &m_componentArray[it->second];
}

template<Serializable T>
Entity ComponentManager<T>::GetEntity(size_t p_index) const {
    DEV_ASSERT(p_index < m_entityArray.size());
    return m_entityArray[p_index];
}

template<Serializable T>
T& ComponentManager<T>::Create(const Entity& p_entity) {
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

template<Serializable T>
bool ComponentManager<T>::Serialize(Archive& p_archive, uint32_t p_version) {
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
            p_archive << entity;
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
            m_componentArray[i].OnDeserialized();
        }
        for (size_t i = 0; i < count; ++i) {
            p_archive >> m_entityArray[i];
            m_lookup[m_entityArray[i]] = i;
        }
    }

    return true;
}

}  // namespace my::ecs
