#include "monte_carlo_tree_search.h"
#include "reversi_game.h"
#include "pgbar/ProgressBar.hpp"
#include "pgbar/BlockBar.hpp"

#include <random>
#include <iostream>
#include <chrono>
#include <functional>

namespace {
//    std::mt19937 gen(std::random_device{}());
    std::default_random_engine gen;
}

std::vector<std::pair<int, int>> GetEmptyPos(const GameState &board_state) {
    std::vector<std::pair<int, int>> empty_pos;
    for (int i = 0; i < board_state.size(); ++i) {
        for (int j = 0; j < board_state[i].size(); ++j) {
            if (board_state[i][j] == Stone::EMPTY) {
                empty_pos.emplace_back(i, j);
            }
        }
    }
    return empty_pos;
}

bool CheckPositionValidMove(Stone player_stone, const GameState &board_state, const std::pair<int, int> &pos) {
    Stone opponent_stone = ReversiGame::GetOpponentStone(player_stone);
    const std::vector<std::pair<int, int>> directions{
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},          {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    for (const auto &dir : directions) {
        int nx = pos.first + dir.first;
        int ny = pos.second + dir.second;
        bool has_opponent_stone_between = false;
        while (nx >= 0 && nx < board_state.size() && ny >= 0 && ny < board_state.size()) {
            if (board_state[nx][ny] == opponent_stone) {
                has_opponent_stone_between = true;
            } else if (board_state[nx][ny] == player_stone) {
                if (has_opponent_stone_between) {
                    return true;
                }
                break;
            } else {
                break;
            }
            nx += dir.first;
            ny += dir.second;
        }
    }
    return false;
}

std::vector<std::pair<int, int>> GetValidMovesFromHint(Stone player_stone, const GameState &board_state,
    const std::set<std::pair<int, int>> &empty_pos) {
    std::vector<std::pair<int, int>> valid_moves;
    for (const auto &pos : empty_pos) {
        if (CheckPositionValidMove(player_stone, board_state, pos)) {
            valid_moves.emplace_back(pos);
        }
    }
    return valid_moves;
}

/**
 * Return winner.
 */
Stone GetGameWinner(const GameState &board_state) {
    int black_count = 0;
    int white_count = 0;
    for (const auto &row : board_state) {
        for (const auto &stone : row) {
            if (stone == Stone::BLACK) {
                black_count++;
            } else if (stone == Stone::WHITE) {
                white_count++;
            }
        }
    }
    if (black_count == white_count) {
        return Stone::EMPTY;
    } else if (black_count > white_count) {
        return Stone::BLACK;
    } else {
        return Stone::WHITE;
    }
}

std::pair<int, int> MonteCarloTreeSearch::SearchMove(const GameState &board_state,
    Stone next_move_stone, int simulation_count, std::vector<std::tuple<int, int, double>> *move_win_ratio)
{
    // pgbar::ProgressBar<> pbar;
    pgbar::BlockBar<> pbar;
    pbar.config().prefix("search move ");
    pbar.config().style( pgbar::config::Line::Entire ).tasks(simulation_count);

    auto time1 = std::chrono::steady_clock::now();
    root = std::make_shared<TreeNode>(board_state, nullptr, next_move_stone, std::pair<int, int>(-1, -1));
    for (int i = 0; i < simulation_count; ++i) {
        pbar.tick();
        auto node = Selection();
        if (node == nullptr) continue;
        ExpandNode(node);
        auto leaf = *(node->children.begin());
        auto winner = Simulate(leaf->state, leaf->next_move_stone);
        BackPropagate(leaf, winner);
    }
    auto time2 = std::chrono::steady_clock::now();
    std::cout << "\nAI think time: " << std::chrono::duration<double>(time2 - time1).count() << "s" << std::endl;
    if (move_win_ratio != nullptr) {
        move_win_ratio->clear();
        for (const auto &ch : root->children) {
            move_win_ratio->emplace_back(ch->from_move.first, ch->from_move.second, ch->win_count / ch->visit_count);
        }
    }
    return GetBestMove();
}

/**
 * Return the node selected. If the node is end of game, return nullptr.
 */
std::shared_ptr<TreeNode> MonteCarloTreeSearch::Selection()
{
    auto node = root;
    while (!node->children.empty()) {
        node = *std::max_element(node->children.begin(), node->children.end(),
            [](const std::shared_ptr<TreeNode> &lhs, const std::shared_ptr<TreeNode> &rhs) {
                return lhs->GetExploitPriority() < rhs->GetExploitPriority();
            });
    }
    auto empty_pos = GetEmptyPos(node->state);
    std::set<std::pair<int, int>> empty_pos_set(empty_pos.begin(), empty_pos.end());
    if (empty_pos.empty() || (GetValidMovesFromHint(node->next_move_stone, node->state, empty_pos_set).empty() &&
        GetValidMovesFromHint(ReversiGame::GetOpponentStone(node->next_move_stone), node->state, empty_pos_set).empty()) ) {
        Stone winner = GetGameWinner(node->state);
        BackPropagate(node, winner);
        return nullptr;
    }
    return node;
}

void MonteCarloTreeSearch::ExpandNode(const std::shared_ptr<TreeNode> &node)
{
    auto empty_pos = GetEmptyPos(node->state);
    std::set<std::pair<int, int>> empty_pos_set(empty_pos.begin(), empty_pos.end());
    auto valid_moves = GetValidMovesFromHint(node->next_move_stone, node->state, empty_pos_set);
    if (valid_moves.empty()) {
        node->next_move_stone = ReversiGame::GetOpponentStone(node->next_move_stone);
        valid_moves = GetValidMovesFromHint(node->next_move_stone, node->state, empty_pos_set);
    }
    for (const auto &move : valid_moves) {
        GameState new_state = node->state;
        ReversiGame::UpdateBoardWithPlacementStone(new_state, move.first, move.second, node->next_move_stone);
        auto new_node = std::make_shared<TreeNode>(new_state, node, ReversiGame::GetOpponentStone(node->next_move_stone), move);
        node->children.emplace_back(new_node);
    }
}

void MonteCarloTreeSearch::BackPropagate(const std::shared_ptr<TreeNode> &node, Stone win_stone)
{
    auto cur_node = node;
    while (cur_node != root) {
        cur_node->visit_count++;
        if (cur_node->parent->next_move_stone == win_stone) {
            cur_node->win_count++;
        } else if (win_stone == Stone::EMPTY){
            cur_node->win_count += 0.5;
        }
        cur_node = cur_node->parent;
    }
    root->visit_count++;
}

/**
 * Return winner.
 */
Stone MonteCarloTreeSearch::Simulate(const GameState &board_state, Stone next_move_stone)
{
    std::vector<std::pair<int, int>> direction = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},          {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    GameState board_state_copy = board_state;
    auto empty_pos = GetEmptyPos(board_state);
    std::set<std::pair<int, int>> empty_pos_set(empty_pos.begin(), empty_pos.end());
    auto cur_move_stone = next_move_stone;
    while(true) {
        auto valid_moves = GetValidMovesFromHint(cur_move_stone, board_state_copy, empty_pos_set);
        if (valid_moves.empty()) {
            cur_move_stone = ReversiGame::GetOpponentStone(cur_move_stone);
            valid_moves = GetValidMovesFromHint(cur_move_stone, board_state_copy, empty_pos_set);
            if (valid_moves.empty()) {
                return GetGameWinner(board_state_copy);
            }
        }
        auto move = valid_moves[gen() % valid_moves.size()];
        ReversiGame::UpdateBoardWithPlacementStone(board_state_copy, move.first, move.second, cur_move_stone);
        empty_pos_set.erase(move);
        cur_move_stone = ReversiGame::GetOpponentStone(cur_move_stone);
        for (const auto &dir: direction) {
            int nx = move.first + dir.first;
            int ny = move.second + dir.second;
            if (nx >= 0 && nx < board_state.size() && ny >= 0 && ny < board_state.size()) {
                if (board_state_copy[nx][ny] == Stone::EMPTY) {
                    empty_pos_set.emplace(nx, ny);
                }
            }
        }
    }
}

std::pair<int, int> MonteCarloTreeSearch::GetBestMove()
{
    std::shared_ptr<TreeNode> best_node = *std::max_element(root->children.begin(), root->children.end(),
        [](const std::shared_ptr<TreeNode> &lhs, const std::shared_ptr<TreeNode> &rhs) {
            return lhs->visit_count < rhs->visit_count;
        });
    std::cout << "win ratio: " << best_node->win_count << "/" << best_node->visit_count
        << " = " << best_node->win_count / best_node->visit_count << std::endl;
    for (const auto &ch : root->children) {
        std::cout << "[" << static_cast<char>(ch->from_move.first + 'A') << ch->from_move.second << ":" << ch->win_count << "/" << ch->visit_count
            << "=" << ch->win_count / ch->visit_count << "] ";
    }
    std::cout << "\nroot visit count: " << root->visit_count << std::endl;
    return best_node->from_move;
}

std::vector<int> MonteCarloTreeSearch::StatDepthNodesNumbers() const {
    std::vector<int> depth_nodes_numbers;
    std::function<void(const TreeNode &, int)> dfs =
        [&](const TreeNode &node, int depth) {
            if (depth >= depth_nodes_numbers.size()) {
                depth_nodes_numbers.push_back(0);
            }
            depth_nodes_numbers[depth] += 1;
            for (const auto &ch : node.children) {
                dfs(*ch, depth + 1);
            }
        };
    dfs(*root, 0);
    return depth_nodes_numbers;
}

int MonteCarloTreeSearch::GetTreeNodesNumbers_(const TreeNode &node) const {
    int count = 1;
    for (auto &ch : node.children) {
        count += GetTreeNodesNumbers_(*ch);
    }
    return count;
}

int MonteCarloTreeSearch::GetTreeDepth_(const TreeNode &node) const {
    int depth = 0;
    for (auto &ch : node.children) {
        depth = std::max(depth, GetTreeDepth_(*ch));
    }
    return depth + 1;
}