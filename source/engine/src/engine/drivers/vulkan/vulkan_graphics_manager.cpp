#include "vulkan_graphics_manager.h"

namespace my {

VulkanGraphicsManager::VulkanGraphicsManager() : EmptyGraphicsManager("VulkanGraphicsManager", Backend::VULKAN, NUM_FRAMES_IN_FLIGHT) {}

}  // namespace my
