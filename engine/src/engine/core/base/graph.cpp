#include "graph.h"

namespace my {

void GraphBase::SetNodeCount(int p_count) {
    DEV_ASSERT(p_count > 0);
    m_vertexCount = p_count;
    m_adjList.resize(p_count);
}

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

bool GraphBase::Sort(std::vector<int>& p_stack) const {
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

void GraphBase::SortUtil(int p_from, std::vector<uint8_t>& p_visited, std::vector<int>& p_topological_stack) const {
    p_visited[p_from] = true;
    for (int to : m_adjList[p_from]) {
        if (!p_visited[to]) {
            SortUtil(to, p_visited, p_topological_stack);
        }
    }

    p_topological_stack.push_back(p_from);
}

}  // namespace my
