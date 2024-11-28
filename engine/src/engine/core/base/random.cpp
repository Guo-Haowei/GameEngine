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

}  // namespace my
