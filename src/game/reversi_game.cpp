#include "reversi_game.h"
#include "monte_carlo_tree_search.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <utility>
#include <thread>

void ReversiGame::MainLoop()
{
    auto [win_pos, win_sz] = game_ui.DrawMainPanel(*this);
    game_ui.DrawHintTextPanel(*this);

    auto &io = ImGui::GetIO();
    // prepare draw board
    ImVec2 left_top(win_pos.x + win_sz.x, 0);
    ImVec2 right_btm(io.DisplaySize.x, io.DisplaySize.y);
    ImVec2 center(0.5 * (left_top.x + right_btm.x), 0.5 * (left_top.y + right_btm.y));
    float board_draw_sz = std::min(right_btm.x - left_top.x, right_btm.y - left_top.y);
    board_draw_sz *= 0.89 * 0.5;
    game_ui.DrawBoard(*this, ImVec2(center.x-board_draw_sz, center.y-board_draw_sz),
        ImVec2(center.x+board_draw_sz, center.y+board_draw_sz));

    // Debug Menu
    if constexpr (true) {
        if (ImGui::Button("Reload config")) {
            LoadConfig();
        }
        auto imcol_to_array = [&](ImColor &col, float ret[3]) {
            ret[0] = col.Value.x;
            ret[1] = col.Value.y;
            ret[2] = col.Value.z;
        };
        float bg_col[3];
        imcol_to_array(game_ui.hint_valid_move_col, bg_col);
        if (ImGui::ColorEdit3("bg color", bg_col, ImGuiColorEditFlags_Float)) {
            game_ui.hint_valid_move_col = ImColor(ImVec4(bg_col[0], bg_col[1], bg_col[2], 1.0));
        }
        if (ImGui::Button("dump config")) {
            DumpConfig();
        }
        if (ImGui::Button("Set game over")) {
            GameConclude();
        }
    }

    switch (game_state_) {
        case GameState::PLAYING:
            if (!is_player_turn_ && ai_think_finish) {
                SearchMove();
            }
            HandleUserInput();
            break;
        case GameState::GAME_OVER:
        default:
            if (!game_over_popup_opened_once_) {
                ImGui::OpenPopup("Game Over");
                game_over_popup_opened_once_ = true;
            }
            break;
    }

    if (ImGui::BeginPopupModal("Game Over")) {
        ImGui::Text("Game Over, %s", hint_text_.c_str());
        ImVec2 btn_sz(80, 25);
        float width = ImGui::GetWindowWidth();
        float btn_interval = 10;
        ImGui::SetCursorPosX((width - btn_sz.x * 2 - btn_interval) * 0.5);
        if (ImGui::Button("Retry", btn_sz)) {
            InitialGame();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX((width + btn_interval) * 0.5);
        if (ImGui::Button("Close", btn_sz)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

}

void ReversiGame::LoadConfig()
{
    try {
        config = YAML::LoadFile("config.yaml");
    } catch(...) {
        throw std::runtime_error("Failed when open config.yaml");
    }
    if (config.IsNull() || !config.IsDefined()) {
        throw std::runtime_error("config is null");
    }

    if (config["ui"]) {
        game_ui.LoadConfig(config["ui"]);
    } else {
        std::cout << "no 'ui' params in config, use default values" << std::endl;
    }

    if (config["board_size"]) {
        board_size_ = config["board_size"].as<int>();
    }
}

void ReversiGame::DumpConfig(const char *dump_config_filename)
{
    YAML::Node node;
    auto node_ui = node["ui"];
    game_ui.DumpConfig(node_ui);

    node["board_size"] = board_size_;

    std::ofstream fout{dump_config_filename};
    fout << node << std::endl;
}

void ReversiGame::ResetIsMoveValid()
{
    is_move_valid_ = std::vector<std::vector<bool>>(board_size_, std::vector<bool>(board_size_, false));
    for (const auto &move : valid_moves_) {
        is_move_valid_[move.first][move.second] = true;
    }
}

void ReversiGame::GameConclude()
{
    game_state_ = GameState::GAME_OVER;
    if (count_black_ == count_white_) {
        hint_text_ = "Game Draw";
    } else if ((this_game_player_first && count_black_ > count_white_) ||
               (!this_game_player_first && count_white_ > count_black_)) {
        hint_text_ = hint_player_win;        
    } else {
        hint_text_ = hint_player_loss;
    }
}


void ReversiGame::PlaceStone(int grid_x, int grid_y)
{
    hint_player_move = false;
    UpdateBoardWithPlacementStone(board_state_, grid_x, grid_y, next_move_stone_);
    UpdateStoneCount();
    record_move_.emplace_back(grid_x, grid_y);
    record_board_state_.emplace_back(board_state_);

    Stone opp_stone = GetOpponentStone(next_move_stone_);
    valid_moves_ = GetValidMoves(opp_stone, board_state_);
    if (valid_moves_.empty()) {
        valid_moves_ = GetValidMoves(next_move_stone_, board_state_);
        if (valid_moves_.empty()) {
            GameConclude();
            return;
        }
    } else {
        is_player_turn_ = !is_player_turn_;
        next_move_stone_ = opp_stone;
    }
    hint_text_ = is_player_turn_ ? hint_players_turn : hint_computer_turn;
    ResetIsMoveValid();
}

void ReversiGame::HandleUserInput()
{
    if (!is_player_turn_ || game_state_ != GameState::PLAYING) {
        return;
    }

    auto &io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }
    if (io.MouseClicked[0] &&
        io.MousePos.x >= game_ui.board_left_top_pos.x && io.MousePos.x <= game_ui.board_right_btm_pos.x &&
        io.MousePos.y >= game_ui.board_left_top_pos.y && io.MousePos.y <= game_ui.board_right_btm_pos.y) {
        int grid_x = static_cast<int>((io.MousePos.x - game_ui.board_left_top_pos.x) / game_ui.line_interval);
        int grid_y = static_cast<int>((io.MousePos.y - game_ui.board_left_top_pos.y) / game_ui.line_interval);
        if (is_move_valid_[grid_x][grid_y]) {
            PlaceStone(grid_x, grid_y);
        }
    }
}

void ReversiGame::UpdateStoneCount()
{
    count_white_ = 0;
    count_black_ = 0;
    for (int i = 0; i < board_size_; ++i) {
        for (int j = 0; j < board_size_; ++j) {
            if (board_state_[i][j] == Stone::WHITE) {
                ++count_white_;
            } else if (board_state_[i][j] == Stone::BLACK) {
                ++count_black_;
            }
        }
    }
}

void ReversiGame::SearchMove(bool place_stone)
{
    ai_think_finish = false;
    for (auto &th : ai_think_threads_) {
        th.join();
    }
    ai_think_threads_.clear();
    ai_think_threads_.emplace_back(std::thread([this, place_stone]() mutable {
        auto move = mcts_.SearchMove(board_state_, next_move_stone_, monte_carlo_iter_steps_, &hint_move_win_ratio);
        std::cout << "num nodes: " << mcts_.GetTreeNodesNumbers() << std::endl;
        std::cout << "depth: " << mcts_.GetTreeDepth() << std::endl;
        std::cout << "node in each depth: [";
        auto list_depth = mcts_.StatDepthNodesNumbers();
        for (int i = 0; i < list_depth.size(); ++i) {
            std::cout << list_depth[i] << ", ";
        }
        std::cout << "]" << std::endl;
        if (place_stone) {
            PlaceStone(move.first, move.second);
        } else {
            hint_player_move = true;
            hint_move_pos = move;
        }
        ai_think_finish = true;
    }));
}

void ReversiGame::WithdrawAMove()
{
    if (!is_player_turn_ || record_move_.empty()) {
        return;
    }
    Stone player_stone = this_game_player_first ? Stone::BLACK : Stone::WHITE;
    while (!record_board_state_.empty() && record_board_state_.back()[record_move_.back().first][record_move_.back().second] != player_stone) {
        record_move_.pop_back();
        record_board_state_.pop_back();
    }
    if (!record_move_.empty()) {
        record_move_.pop_back();
        record_board_state_.pop_back();
    }
    board_state_ = record_board_state_.back();
    hint_player_move = false;
    UpdateStoneCount();
    valid_moves_ = GetValidMoves(next_move_stone_, board_state_);
    ResetIsMoveValid();
}

void ReversiGame::InitialGame()
{
    game_state_ = GameState::PLAYING;
    record_move_.clear();
    record_board_state_.clear();
    board_state_.clear();
    board_state_ = std::vector<std::vector<Stone>>(board_size_, std::vector<Stone>(board_size_, Stone::EMPTY));
    // reset board state
    int initial_stone_pos = board_size_ / 2 - 1;
    board_state_[initial_stone_pos][initial_stone_pos] =
        board_state_[initial_stone_pos+1][initial_stone_pos+1] = Stone::BLACK;
    board_state_[initial_stone_pos+1][initial_stone_pos] =
        board_state_[initial_stone_pos][initial_stone_pos+1] = Stone::WHITE;
    
    next_move_stone_ = Stone::BLACK;
    valid_moves_ = GetValidMoves(next_move_stone_, board_state_);
    UpdateStoneCount();
    ResetIsMoveValid();

    this_game_player_first = next_game_player_first;
    is_player_turn_ = next_game_player_first;
    hint_text_ = is_player_turn_ ? hint_players_turn : hint_computer_turn;

    ai_think_finish = true;
    hint_player_move = false;
    record_board_state_.emplace_back(board_state_);
    game_over_popup_opened_once_ = false;
}


std::vector<std::pair<int, int>> ReversiGame::GetValidMoves(Stone player_stone, const std::vector<std::vector<Stone>> &board_state)
{
    std::vector<std::pair<int, int>> valid_moves;
    int board_size = static_cast<int>(board_state.size());
    Stone opponent_stone = (player_stone == Stone::BLACK) ? Stone::WHITE : Stone::BLACK;
    const std::vector<std::pair<int, int>> directions{
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},          {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };

    for (int x = 0; x < board_size; ++x) {
        for (int y = 0; y < board_size; ++y) {
            if (board_state[x][y] != Stone::EMPTY) {
                continue;
            }
            bool is_valid = false;
            for (const auto &dir : directions) {
                int nx = x + dir.first;
                int ny = y + dir.second;
                bool has_opponent_stone_between = false;
                while (nx >= 0 && nx < board_size && ny >= 0 && ny < board_size) {
                    if (board_state[nx][ny] == opponent_stone) {
                        has_opponent_stone_between = true;
                    } else if (board_state[nx][ny] == player_stone) {
                        if (has_opponent_stone_between) {
                            is_valid = true;
                        }
                        break;
                    } else {
                        break;
                    }
                    nx += dir.first;
                    ny += dir.second;
                }
                if (is_valid) {
                    valid_moves.emplace_back(x, y);
                    break;
                }
            }
        }
    }
    return valid_moves;
}

Stone ReversiGame::GetOpponentStone(Stone stone)
{
    return stone == Stone::WHITE ? Stone::BLACK : Stone::WHITE;
}

void ReversiGame::UpdateBoardWithPlacementStone(std::vector<std::vector<Stone>> &board_state, int place_x, int place_y,
                                                Stone place_stone)
{
    int board_sz = board_state.size();
    board_state[place_x][place_y] = place_stone;
    const std::vector<std::pair<int, int>> directions{
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},          {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    Stone opp_stone = GetOpponentStone(place_stone);
    for (auto &dir : directions) {
        bool is_opp_between = false;
        int pos_x = place_x + dir.first;
        int pos_y = place_y + dir.second;
        while (pos_x >= 0 && pos_x < board_sz && pos_y >= 0 && pos_y < board_sz
               && board_state[pos_x][pos_y] == opp_stone) {
            pos_x += dir.first;
            pos_y += dir.second;
        }
        if (pos_x >= 0 && pos_x < board_sz && pos_y >= 0 && pos_y < board_sz
            && board_state[pos_x][pos_y] == place_stone) {
            while (pos_x != place_x || pos_y != place_y) {
                board_state[pos_x][pos_y] = place_stone;
                pos_x -= dir.first;
                pos_y -= dir.second;
            }
        }
    }
}

void ReversiGame::HintPlayerMove()
{
    if (is_player_turn_ && ai_think_finish && !hint_player_move) {
        SearchMove(false);
    } 
}
