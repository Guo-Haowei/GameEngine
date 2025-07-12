#pragma once

namespace my {

class GraphBase {
public:
    void SetNodeCount(int p_count);

    void AddEdge(int p_from, int p_to);

    bool HasEdge(int p_from, int p_to) const;

    bool HasCycle() const;

    bool IsReachable(int p_from, int p_to) const;

    const auto& GetAdjList() const { return m_adjList; }

    bool Sort(std::vector<int>& p_stack) const;

protected:
    void SortUtil(int p_from, std::vector<uint8_t>& p_visited, std::vector<int>& p_topological_stack) const;

    std::vector<std::unordered_set<int>> m_adjList;

    int m_vertexCount{ 0 };
};

template<typename T>
class Graph : public GraphBase {
public:
#pragma region ITERATOR
    class Iterator {
        using Self = Iterator;

    public:
        Iterator(const std::vector<T>& p_vertices,
                 const std::vector<int>& p_sorted,
                 size_t p_index)
            : m_vertices(p_vertices), m_sorted(p_sorted), m_index(p_index) {}

        Self operator++(int) {
            Self tmp = *this;
            ++m_index;
            return tmp;
        }

        Self operator--(int) {
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

    void SetNodeCount(int p_count) {
        GraphBase::SetNodeCount(p_count);
        m_vertices.reserve(p_count);
        m_sorted.reserve(p_count);
    }

    bool Compile() { return GraphBase::Sort(m_sorted); }

    const auto& GetSortedOrder() const { return m_sorted; }
    const auto& GetVertices() const { return m_vertices; }

private:
    std::vector<T> m_vertices;
    std::vector<int> m_sorted;
};

}  // namespace my
