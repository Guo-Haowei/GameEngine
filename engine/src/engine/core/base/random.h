#include <random>

namespace my {

class Random {
public:
    static void Initialize();

    static float Float();

    static float Float(float p_min, float p_max);

private:
    static std::mt19937 s_randomEngine;
    static std::uniform_real_distribution<float> s_distribution;
};

}  // namespace my
