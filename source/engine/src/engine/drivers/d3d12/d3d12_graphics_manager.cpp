#include "d3d12_graphics_manager.h"

#include "drivers/d3d12/d3d12_pipeline_state_manager.h"

namespace my {

D3d12GraphicsManager::D3d12GraphicsManager() : EmptyGraphicsManager("D3d12GraphicsManager", Backend::D3D12) {
    m_pipelineStateManager = std::make_shared<D3d12PipelineStateManager>();
}

}  // namespace my
