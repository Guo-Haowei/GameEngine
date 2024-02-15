#pragma once
#include "image.h"

namespace my {

std::shared_ptr<Image> load_image(const std::string& path);

}  // namespace my