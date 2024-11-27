#include "metal_graphics_manager.h"

#include "drivers/empty/empty_pipeline_state_manager.h"

namespace my {

MetalGraphicsManager::MetalGraphicsManager() : EmptyGraphicsManager("MetalGraphicsManager", Backend::METAL, NUM_FRAMES_IN_FLIGHT) {
    m_pipelineStateManager = std::make_shared<EmptyPipelineStateManager>();
}

}
