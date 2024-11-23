#pragma once
#include "core/framework/graphics_manager.h"

// @TODO: remove
#include "drivers/empty/empty_graphics_manager.h"

namespace my {

class VulkanGraphicsManager : public EmptyGraphicsManager {
public:
    VulkanGraphicsManager();
};

}  // namespace my
