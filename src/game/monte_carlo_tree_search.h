#ifndef __MONTE_CARLO_TREE_SEARCH_H__
#define __MONTE_CARLO_TREE_SEARCH_H__

#include "game_const.h"

#include <vector>
#include <utility>
#include <memory>
#include <limits>
#include <cmath>
#include <queue>

using GameState = std::vector<std::vector<Stone>>;

struct TreeNode {
    GameState state;
    std::shared_ptr<TreeNode> parent = nullptr;
    Stone next_move_stone;
    std::pair<int, int> from_move;

    struct NodeComparator {
        bool operator()(const std::shared_ptr<TreeNode> &lhs, const std::shared_ptr<TreeNode> &rhs) {
            return lhs->exploit_priority < rhs->exploit_priority;
        }
    };

    std::priority_queue<std::shared_ptr<TreeNode>, std::vector<std::shared_ptr<TreeNode>>, NodeComparator> children;
    int visit_count = 0;
    double win_count = 0;
    double exploit_priority = 0.0;

    TreeNode(const GameState &state, std::shared_ptr<TreeNode> parent, Stone next_move_stone,
                const std::pair<int, int> &from_move) : state{state},
                parent{parent}, next_move_stone{next_move_stone}, from_move{from_move} {
        UpdateExploitPriority();
    }

    void UpdateExploitPriority() {
        if (parent == nullptr) {
            return;
        }
        exploit_priority = ComputeExploitPriority(parent->visit_count);
    }


private:
    double ComputeExploitPriority(int parent_total_rounds) {
        const double coef = 1.4142135623730951; // sqrt(2)
        if (visit_count == 0) {
            return std::numeric_limits<double>::infinity();
        }
        double win_ratio = win_count / visit_count;
        double exploit = coef * std::sqrt(std::log(parent_total_rounds) / visit_count);
        return win_ratio + exploit;
    }
};

class MonteCarloTreeSearch {
public:
    MonteCarloTreeSearch() = default;
    std::pair<int, int> SearchMove(const GameState &board_state, Stone next_move_stone, int simulation_count = 10000);

    int GetTreeNodesNumbers() {
        return GetTreeNodesNumbers_(*root);
    }

    int GetTreeDepth() {
        return GetTreeDepth_(*root);
    }

    std::vector<int> StatDepthNodesNumbers();
private:
    std::shared_ptr<TreeNode> Selection();
    void ExpandNode(const std::shared_ptr<TreeNode> &node);
    void BackPropagate(const std::shared_ptr<TreeNode> &node, Stone win_stone);
    Stone Simulate(const GameState &board_state, Stone next_move_stone);
    std::pair<int, int> GetBestMove();
    std::shared_ptr<TreeNode> root;
    int GetTreeNodesNumbers_(TreeNode &node);
    int GetTreeDepth_(TreeNode &node);
};

#endif