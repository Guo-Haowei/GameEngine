#include "display_manager.h"
#include "graphics_manager.h"
#include "physics_manager.h"

namespace my {

DisplayManager* CreateDisplayManager();

IGraphicsManager* CreateGraphicsManager();

IPhysicsManager* CreatePhysicsManager();

}  // namespace my
