#ifndef __REVERSI_GAME_H__
#define __REVERSI_GAME_H__

#include "game_ui.h"
#include "game_const.h"
#include "monte_carlo_tree_search.h"

#include "imgui.h"
#include "SDL.h"
#include "yaml-cpp/yaml.h"

#include <vector>
#include <utility>
#include <atomic>
#include <thread>

class ReversiGame {
public:
    enum class GameState {
        PLAYING,
        GAME_OVER
    };
    

    static ReversiGame &GetInstance() {
        static ReversiGame instance;
        return instance;
    }

    void MainLoop();

    GameUI game_ui;

    static std::vector<std::pair<int, int>> GetValidMoves(Stone player_stone, const std::vector<std::vector<Stone>> &board_state);
    static Stone GetOpponentStone(Stone stone);
    static void UpdateBoardWithPlacementStone(std::vector<std::vector<Stone>> &board_state, int place_x, int place_y, Stone place_stone);

    ~ReversiGame() {
        for (auto &t: ai_think_threads_) {
            t.join();
        }
    }
private:
    ReversiGame() {
        LoadConfig();
        InitialGame();
    }

    float GetFps() {
        static Uint64 last_tick = 0;
        Uint64 tick = SDL_GetTicks64();
        float fps = 1000.0f / (tick - last_tick);
        last_tick = tick;
        return fps;
    }

    void InitialGame();

    void LoadConfig();
    void DumpConfig(const char *dump_config_filename = "dump_config.yaml");
    void ResetIsMoveValid();
    void GameConclude();
    void PlaceStone(int grid_x, int grid_y);

    void HandleUserInput();
    void UpdateStoneCount();

    void SearchMove();

    YAML::Node config;

    int board_size_ = 8;
    std::vector<std::vector<Stone>> board_state_;
    Stone next_move_stone_ = Stone::BLACK;
    std::vector<std::pair<int, int>> valid_moves_;
    std::vector<std::vector<bool>> is_move_valid_;
    bool next_game_player_first = true;
    bool is_player_turn_ = true;
    bool this_game_player_first = true;
    GameState game_state_ = GameState::PLAYING;

    std::vector<std::pair<int,int>> record_move_;

    std::string hint_text_;

    int count_black_ = 0;
    int count_white_ = 0;
    friend GameUI;

    const std::string hint_players_turn = "player's turn";
    const std::string hint_computer_turn = "AI is thinking ... ";
    const std::string hint_player_win = "You win";
    const std::string hint_player_loss = "You loss";  
    
    MonteCarloTreeSearch mcts_;
    std::atomic<bool> ai_think_finish = true;
    int monte_carlo_iter_steps_ = 60000;
    std::vector<std::thread> ai_think_threads_;
};

#endif