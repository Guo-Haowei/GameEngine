#include "display_manager.h"
#include "graphics_manager_interface.h"
#include "physics_manager.h"

namespace my {

DisplayManager* CreateDisplayManager();

IGraphicsManager* CreateGraphicsManager();

IPhysicsManager* CreatePhysicsManager();

}  // namespace my
