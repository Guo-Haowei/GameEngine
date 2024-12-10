#include "random.h"

namespace my {

std::mt19937 Random::s_randomEngine;
std::uniform_real_distribution<float> Random::s_distribution(0.0f, 1.0f);

void Random::Initialize() {
    s_randomEngine.seed(std::random_device()());
}

float Random::Float() {
    return s_distribution(s_randomEngine);
}

float Random::Float(float p_min, float p_max) {
    DEV_ASSERT(p_max >= p_min);
    return p_min + (p_max - p_min) * Float();
}

}  // namespace my
