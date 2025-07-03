#include "physics_manager.h"
#include "modules/bullet_physics/bullet_physics_manager.h"

namespace my {

PhysicsManager* PhysicsManager::Create() {
    // @TODO: configure what manager to use
    return new Bullet3PhysicsManager();
}

}  // namespace my

