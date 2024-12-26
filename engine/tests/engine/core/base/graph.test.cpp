#include "engine/core/base/graph.h"

namespace my {

/*
A - B
| / |
C - D
*/
static void BuildInvalidGraph1(GraphBase& p_graph) {
    enum {
        A = 0,
        B,
        C,
        D,
        MAX,
    };
    p_graph.SetNodeCount(MAX);
    p_graph.AddEdge(A, B);
    p_graph.AddEdge(B, D);
    p_graph.AddEdge(D, C);
    p_graph.AddEdge(C, A);
    p_graph.AddEdge(C, B);
}

/*
A -> B \
        E -> F
C -> D /
*/
static void BuildValidGraph1(GraphBase& p_graph) {
    enum {
        A = 0,
        B,
        C,
        D,
        E,
        F,
        MAX,
    };
    p_graph.SetNodeCount(MAX);
    p_graph.AddEdge(A, B);
    p_graph.AddEdge(C, D);
    p_graph.AddEdge(B, E);
    p_graph.AddEdge(D, E);
    p_graph.AddEdge(E, F);
}

/*
A -> B \
|------> E -> F
         |    |
         C -> D
*/
static void BuildValidGraph2(GraphBase& p_graph) {
    enum {
        A = 0,
        B,
        C,
        D,
        E,
        F,
        MAX,
    };
    p_graph.SetNodeCount(MAX);
    p_graph.AddEdge(A, B);
    p_graph.AddEdge(A, E);
    p_graph.AddEdge(B, E);
    p_graph.AddEdge(E, F);
    p_graph.AddEdge(E, C);
    p_graph.AddEdge(C, D);
    p_graph.AddEdge(D, F);
}

TEST(GraphBase, check_cycle_valid) {
    GraphBase graph;
    BuildValidGraph1(graph);
    EXPECT_FALSE(graph.HasCycle());
}

TEST(GraphBase, check_cycle_invalid) {
    GraphBase graph;
    BuildInvalidGraph1(graph);
    EXPECT_TRUE(graph.HasCycle());
}

static void CheckSorted(const GraphBase& p_graph, const std::vector<int>& p_sorted) {
#if 0
    for (auto i : p_sorted) {
        printf("%c -> ", i + 'A');
    }
    printf("\n");
#endif

    for (size_t i = 0; i < p_sorted.size() - 1; ++i) {
        int from = p_sorted[i];
        for (size_t j = i + 1; j < p_sorted.size(); ++j) {
            int to = p_sorted[j];
            EXPECT_FALSE(p_graph.IsReachable(to, from)) << "Test failed! There's a path from " << char(to + 'A') << " to " << char(from + 'A');
        }
    }
}

TEST(GraphBase, topological_sort_1) {
    GraphBase graph;
    BuildValidGraph1(graph);

    std::vector<int> sorted;
    EXPECT_TRUE(graph.Sort(sorted));

    CheckSorted(graph, sorted);
}

TEST(GraphBase, topological_sort_2) {
    GraphBase graph;
    BuildValidGraph2(graph);

    std::vector<int> sorted;
    EXPECT_TRUE(graph.Sort(sorted));

    CheckSorted(graph, sorted);
}

TEST(Graph, sort) {
    enum {
        A = 0,
        B,
        C,
        D,
        E,
        F,
        MAX,
    };

    Graph<char> graph;
    graph.SetNodeCount(MAX);
    graph.AddVertex('A');
    graph.AddVertex('B');
    graph.AddVertex('C');
    graph.AddVertex('D');
    graph.AddVertex('E');
    graph.AddVertex('F');
    graph.AddEdge(A, B);
    graph.AddEdge(A, E);
    graph.AddEdge(B, E);
    graph.AddEdge(E, F);
    graph.AddEdge(E, C);
    graph.AddEdge(C, D);
    graph.AddEdge(D, F);

    graph.Compile();

    EXPECT_FALSE(graph.HasCycle());
}

}  // namespace my
