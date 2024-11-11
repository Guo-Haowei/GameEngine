#pragma once
#include "core/framework/graphics_manager.h"
#include "drivers/empty/empty_graphics_manager.h"

namespace my {

class D3d12GraphicsManager : public EmptyGraphicsManager {
public:
    D3d12GraphicsManager();
};

}  // namespace my
