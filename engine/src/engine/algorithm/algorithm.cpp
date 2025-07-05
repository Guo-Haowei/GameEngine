#include "algorithm.h"

namespace my {

std::vector<int> topological_sort(int N, const std::vector<std::vector<int>>& p_edges) {
    std::vector<int> indegree(N, 0);
    std::vector<int> sorted;
    std::vector<std::vector<int>> adj(N);
    for (const auto& edge : p_edges) {
        const int from = edge[0];
        const int to = edge[1];
        adj[from].push_back(to);
        indegree[to] += 1;
    }

    std::list<int> queue;
    for (int i = 0; i < indegree.size(); ++i) {
        if (indegree[i] == 0) queue.push_back(i);
    }

    while (!queue.empty()) {
        int from = queue.front();
        queue.pop_front();
        sorted.push_back(from);
        for (int to : adj[from]) {
            indegree[to] -= 1;
            if (indegree[to] == 0) {
                queue.push_back(to);
            }
        }
    }

    if (sorted.size() != N) {
        return {};
    }

    return sorted;
}

}  // namespace my
