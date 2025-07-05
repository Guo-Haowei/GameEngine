#include <engine/algorithm/algorithm.h>

namespace my {

TEST(topological_sort, test1) {
    std::vector<std::vector<int>> edges = { { 1, 0 } };

    auto sorted = topological_sort(2, edges);

    EXPECT_EQ(sorted[0], 1);
    EXPECT_EQ(sorted[1], 0);
}

TEST(topological_sort, test2) {
    std::vector<std::vector<int>> edges = {
        { 0, 1 },
        { 0, 2 },
        { 1, 3 },
        { 2, 3 }
    };

    auto sorted = topological_sort(4, edges);

    EXPECT_EQ(sorted[0], 0);
    EXPECT_EQ(sorted[1], 1);
    EXPECT_EQ(sorted[2], 2);
    EXPECT_EQ(sorted[3], 3);
}

}  // namespace my
