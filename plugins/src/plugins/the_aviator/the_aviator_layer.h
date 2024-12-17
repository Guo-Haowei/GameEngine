#pragma once
#include "engine/core/framework/layer.h"
#include "engine/core/math/geomath.h"
#include "the_aviator_defines.h"
#include "engine/systems/ecs/entity.h"

namespace my {

class Scene;

struct GameObject {
    float lifeRemains;
};

template<typename T, int N>
    requires std::is_base_of_v<GameObject, T>
struct ObjectPool {
    std::array<T, N> pool;
    std::list<T*> deadList;
    std::list<T*> aliveList;

    void Clear() {
        deadList.clear();
        aliveList.clear();
    }

    void Free(T* p_object) {
        deadList.push_back(p_object);
    }

    T* Alloc() {
        if (deadList.empty()) {
            return nullptr;
        }

        T* result = deadList.front();
        deadList.pop_front();
        aliveList.push_back(result);
        return result;
    }

    void Update(float p_timestep, std::function<void(T*)> p_on_destroy) {
        std::list<T*> tmp;
        for (int i = (int)aliveList.size() - 1; i >= 0; --i) {
            T* object = aliveList.back();
            aliveList.pop_back();
            object->lifeRemains -= p_timestep;
            if (object->lifeRemains <= 0.0f) {
                p_on_destroy(object);
                deadList.push_back(object);
            } else {
                tmp.emplace_back(object);
            }
        }
        aliveList = std::move(tmp);
    }
};


class TheAviatorLayer : public Layer {
public:
    TheAviatorLayer();

    void OnAttach() override;
    void OnDetach() override;

    void OnUpdate(float p_timestep) override;

private:
    enum : uint32_t {
        ROCK_POOL_SIZE = 16,
        BATTERY_POOL_SIZE = 32,
    };

    struct Rock : GameObject {
        ecs::Entity id;
    };

    struct Battery : GameObject {
        ecs::Entity id;
    };

    static constexpr float ROCK_SIZE = 4.0f;
    static constexpr float BATTERY_SIZE = 2.0f;
    static constexpr float ENTITY_LIFE_TIME = 1.5f * glm::pi<float>() / WORLD_SPEED - 3.0f;

    [[nodiscard]] ecs::Entity CreateRockResource();
    [[nodiscard]] ecs::Entity CreateBatteryResource();

    float m_time;
    float m_lastSpawnTime;

    ObjectPool<Rock, ROCK_POOL_SIZE> m_rockPool;
    ObjectPool<Battery, BATTERY_POOL_SIZE> m_batteryPool;

    Scene* m_scene;
    ecs::Entity m_id;
};
    
} // namespace my
