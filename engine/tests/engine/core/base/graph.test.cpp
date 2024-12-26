#include "engine/core/base/graph.h"

namespace my::internal {

class GraphBase {
public:
    void SetNodeCount(int p_count) {
        DEV_ASSERT(p_count > 0);
        m_vertexCount = p_count;
        m_adjList.resize(p_count);
    }

    void AddEdge(int p_from, int p_to);

    bool HasEdge(int p_from, int p_to) const;

    bool HasCycle() const;

    bool IsReachable(int p_from, int p_to) const;

    const auto& GetAdjList() const { return m_adjList; }

    bool Sort(std::vector<int>& p_stack) const {
        if (HasCycle()) {
            return false;
        }

        std::vector<uint8_t> visited(m_vertexCount, false);

        for (int i = 0; i < m_vertexCount; ++i) {
            if (!visited[i]) {
                SortUtil(i, visited, p_stack);
            }
        }

        std::reverse(p_stack.begin(), p_stack.end());
        return true;
    }

protected:
    void SortUtil(int p_from, std::vector<uint8_t>& p_visited, std::vector<int>& p_topological_stack) const {
        p_visited[p_from] = true;
        for (int to : m_adjList[p_from]) {
            if (!p_visited[to]) {
                SortUtil(to, p_visited, p_topological_stack);
            }
        }

        p_topological_stack.push_back(p_from);
    }

    std::vector<std::unordered_set<int>> m_adjList;

    int m_vertexCount{ 0 };
};

///
template<typename T>
class Graph : public GraphBase {
public:
#pragma region ITERATOR
    class Iterator {
        using Self = Iterator;

    public:
        Iterator(const std::vector<T>& p_vertices,
                 const std::vector<int>& p_sorted,
                 size_t p_index) : m_vertices(p_vertices),
                                   m_sorted(p_sorted),
                                   m_index(p_index) {}

        Self operator++(int) const {
            Self tmp = *this;
            ++m_index;
            return tmp;
        }

        Self operator--(int) const {
            Self tmp = *this;
            --m_index;
            return tmp;
        }

        Self& operator++() {
            ++m_index;
            return *this;
        }

        Self& operator--() {
            --m_index;
            return *this;
        }

        bool operator==(const Self& p_rhs) const { return m_index == p_rhs.m_index; }
        bool operator!=(const Self& p_rhs) const { return m_index != p_rhs.m_index; }

        const T& operator*() const {
            return m_vertices[m_sorted[m_index]];
        }

    private:
        const std::vector<T>& m_vertices;
        const std::vector<int>& m_sorted;
        size_t m_index;
    };
#pragma endregion ITERATOR

    Iterator begin() { return Iterator(m_vertices, m_sorted, 0); }
    Iterator end() { return Iterator(m_vertices, m_sorted, m_sorted.size()); }

    void AddVertex(const T& p_node) {
        DEV_ASSERT_INDEX(m_vertices.size(), m_vertexCount);
        m_vertices.push_back(p_node);
    }

    bool Compile() { return GraphBase::Sort(m_sorted); }

private:
    std::vector<T> m_vertices;
    std::vector<int> m_sorted;
};
///

bool GraphBase::HasEdge(int p_from, int p_to) const {
    DEV_ASSERT_INDEX(p_from, m_vertexCount);
    DEV_ASSERT_INDEX(p_to, m_vertexCount);
    return m_adjList[p_from].find(p_to) != m_adjList[p_from].end();
}

void GraphBase::AddEdge(int p_from, int p_to) {
    DEV_ASSERT_INDEX(p_from, m_vertexCount);
    DEV_ASSERT_INDEX(p_to, m_vertexCount);
    DEV_ASSERT(p_from != p_to);
    DEV_ASSERT(!HasEdge(p_from, p_to));
    m_adjList[p_from].insert(p_to);
    return;
}

bool GraphBase::HasCycle() const {
    for (int i = 0; i < m_vertexCount; ++i) {
        for (int from : m_adjList[i]) {
            if (IsReachable(from, i)) {
                return true;
            }
        }
    }
    return false;
}

bool GraphBase::IsReachable(int p_from, int p_to) const {
    DEV_ASSERT(p_from != p_to);
    DEV_ASSERT_INDEX(p_from, m_vertexCount);
    DEV_ASSERT_INDEX(p_to, m_vertexCount);

    if (p_from == p_to) {
        return true;
    }

    std::queue<int> queue;
    std::vector<bool> visited(m_vertexCount, false);
    visited[p_from] = true;
    queue.push(p_from);

    while (!queue.empty()) {
        p_from = queue.front();
        queue.pop();

        for (int node : m_adjList[p_from]) {
            if (node == p_to) {
                return true;
            }

            if (!visited[node]) {
                visited[node] = true;
                queue.push(node);
            }
        }
    }

    return false;
}

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

#if 0

TEST(graph, sort_level) {
    EXPECT_FALSE(graph.HasCycle());

    auto levels = graph.build_level();

    EXPECT_EQ(levels[0][0], A);
    EXPECT_EQ(levels[0][1], C);
    EXPECT_EQ(levels[1][0], B);
    EXPECT_EQ(levels[1][1], D);
    EXPECT_EQ(levels[2][0], E);
    EXPECT_EQ(levels[3][0], F);
    return;
}
#endif

}  // namespace my::internal
