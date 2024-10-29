#include <random>

namespace my {

class Random {
public:
    static void Initialize();

    static float Float();

private:
    static std::mt19937 s_randomEngine;
    static std::uniform_real_distribution<float> s_distribution;
};

}  // namespace my
