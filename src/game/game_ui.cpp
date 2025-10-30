#include "game_ui.h"
#include "reversi_game.h"

#include "imgui.h"

#include <vector>
#include <utility>

void GameUI::LoadConfig(const YAML::Node &node_ui)
{
    background_col = node_ui["background_col"].as<ImColor>();
    board_fill_col = node_ui["board_fill_col"].as<ImColor>();
    line_col = node_ui["line_col"].as<ImColor>();
    if (node_ui["hint_valid_move_col"]) {
        hint_valid_move_col = node_ui["hint_valid_move_col"].as<ImColor>();
    }
    if (node_ui["hint_mouse_pos_col"]) {
        hint_mouse_pos_col = node_ui["hint_mouse_pos_col"].as<ImColor>();
    }
}

void GameUI::DumpConfig(YAML::Node &node_ui)
{
    node_ui["background_col"] = background_col;
    node_ui["board_fill_col"] = board_fill_col;
    node_ui["line_col"] = line_col;
    node_ui["hint_valid_move_col"] = hint_valid_move_col;
    node_ui["hint_mouse_pos_col"] = hint_mouse_pos_col;
}

void GameUI::DrawBoard(ReversiGame &game, const ImVec2 &left_top_pos, const ImVec2 &right_btm_pos)
{
    this->board_left_top_pos = left_top_pos;
    this->board_right_btm_pos = right_btm_pos;
    auto draw_list = ImGui::GetBackgroundDrawList();
    int boaden_pixel = 20;
    draw_list->AddRectFilled(ImVec2(left_top_pos.x - boaden_pixel, left_top_pos.y - boaden_pixel),
        ImVec2(right_btm_pos.x + boaden_pixel, right_btm_pos.y + boaden_pixel), board_fill_col);
    boaden_pixel = 6;
    draw_list->AddRect(ImVec2(left_top_pos.x - boaden_pixel, left_top_pos.y - boaden_pixel),
        ImVec2(right_btm_pos.x + boaden_pixel, right_btm_pos.y + boaden_pixel), line_col, 0, 0, 3);
    draw_list->AddRect(left_top_pos, right_btm_pos, line_col);

    // draw lines
    line_interval = (right_btm_pos.x - left_top_pos.x) / game.board_size_;
    for (int i = 1; i < game.board_size_; ++i) {
        float y_pos = left_top_pos.y + i * line_interval;
        draw_list->AddLine(ImVec2(left_top_pos.x, y_pos), ImVec2(right_btm_pos.x, y_pos), line_col);
        float x_pos = left_top_pos.x + i * line_interval;
        draw_list->AddLine(ImVec2(x_pos, left_top_pos.y), ImVec2(x_pos, right_btm_pos.y), line_col);
    }

    // draw five dots
    float dot_radius = std::min<float>(5, 0.1*line_interval);
    float dot_pos = (game.board_size_ / 4) * line_interval;
    draw_list->AddCircleFilled(ImVec2(left_top_pos.x + dot_pos, left_top_pos.y + dot_pos), dot_radius, black_col);
    draw_list->AddCircleFilled(ImVec2(left_top_pos.x + dot_pos, right_btm_pos.y - dot_pos), dot_radius, black_col);
    draw_list->AddCircleFilled(ImVec2(right_btm_pos.x - dot_pos, left_top_pos.y + dot_pos), dot_radius, black_col);
    draw_list->AddCircleFilled(ImVec2(right_btm_pos.x - dot_pos, right_btm_pos.y - dot_pos), dot_radius, black_col);
    if (game.board_size_ % 2 == 0) {
        dot_pos = (game.board_size_ / 2) * line_interval;
        draw_list->AddCircleFilled(ImVec2(left_top_pos.x + dot_pos, left_top_pos.y + dot_pos), dot_radius, black_col);
    }

    float stone_radius = line_interval * 0.9f * 0.5f;

    // draw valid moves
    float hint_radius = stone_radius * 0.3f;
    for (const auto &move : game.valid_moves_) {
        draw_list->AddCircleFilled(ImVec2(left_top_pos.x + line_interval * (move.first+0.5), left_top_pos.y + line_interval * (move.second+0.5)),
                                   hint_radius, hint_valid_move_col);
    }


    // draw mouse position hint
    auto &io = ImGui::GetIO();
    if (io.MousePos.x >= left_top_pos.x && io.MousePos.x <= right_btm_pos.x &&
        io.MousePos.y >= left_top_pos.y && io.MousePos.y <= right_btm_pos.y) {
        int grid_x = static_cast<int>((io.MousePos.x - left_top_pos.x) / line_interval);
        int grid_y = static_cast<int>((io.MousePos.y - left_top_pos.y) / line_interval);
        ImVec2 rect_left_top(left_top_pos.x + grid_x * line_interval, left_top_pos.y + grid_y * line_interval);
        ImVec2 rect_right_btm(left_top_pos.x + (grid_x + 1) * line_interval, left_top_pos.y + (grid_y + 1) * line_interval);
        ImU32 hint_stone_col = game.next_move_stone_ == Stone::WHITE ? white_col : black_col;
        hint_stone_col = (hint_stone_col & 0x00FFFFFF) | (0xE0 << 24); // make hint stone col more transparent (Alpha = 0x30 ≈ 19% opacity)

        draw_list->AddRect(rect_left_top, rect_right_btm, hint_mouse_pos_col, 0, 0, 3);
        if (game.is_move_valid_[grid_x][grid_y]) {
            draw_list->AddCircleFilled(ImVec2(left_top_pos.x + (grid_x + 0.5) * line_interval,
                                            left_top_pos.y + (grid_y + 0.5) * line_interval),
                                    stone_radius, hint_stone_col);
        }
    }
   
    // draw stones
    for (int i = 0; i < game.board_size_; ++i) {
        for (int j = 0; j < game.board_size_; ++j) {
            if (game.board_state_[i][j] == Stone::EMPTY) {
                continue;
            }
            draw_list->AddCircleFilled(ImVec2(left_top_pos.x + line_interval * (i+0.5), left_top_pos.y + line_interval * (j+0.5)),
                                    stone_radius, game.board_state_[i][j] == Stone::WHITE ? white_col : black_col);
        }
    }

    // draw hint last move
    if (game.record_move_.empty()) {
        return;
    }
    ImVec2 hint_center_pos(left_top_pos.x + line_interval * (game.record_move_.back().first + 0.5),
                           left_top_pos.y + line_interval * (game.record_move_.back().second + 0.5));
    float hint_rect_half_size = 5.0f;
    draw_list->AddRectFilled(
        ImVec2(hint_center_pos.x - hint_rect_half_size, hint_center_pos.y - hint_rect_half_size),
        ImVec2(hint_center_pos.x + hint_rect_half_size, hint_center_pos.y + hint_rect_half_size),
        ImGui::GetColorU32(ImVec4(1, 0, 0, 1)), 0, 0);
}

std::pair<ImVec2, ImVec2>  GameUI::DrawMainPanel(ReversiGame &game)
{
    auto &io = ImGui::GetIO();
    ImVec2 win_sz(std::min<float>(200.0f, io.DisplaySize.x * 0.3f), std::min<float>(180.0f, io.DisplaySize.y*0.5f));
    ImVec2 win_pos(10, 10);
    ImGui::SetNextWindowSize(win_sz, ImGuiCond_Always);
    ImGui::SetNextWindowPos(win_pos);
    ImGui::SetNextWindowBgAlpha(win_bg_alpha_);

    ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("fps: %.3f", game.GetFps());

    ImVec2 btn_sz(100, 30);
    ImGui::SetCursorPosX((win_sz.x - btn_sz.x) * 0.5f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
    if (ImGui::Button("New Game", btn_sz)) {
        game.InitialGame();
    }
    ImGui::Text("Who first");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted("Next game who first");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    if (ImGui::RadioButton("Player", game.next_game_player_first)) {
        game.next_game_player_first = true;
    }
    if (ImGui::RadioButton("AI", !game.next_game_player_first)) {
        game.next_game_player_first = false;
    }
    ImGui::Text("Player use %s stone.", game.this_game_player_first ? "black" : "white");

    this->win_pos = ImGui::GetWindowPos();
    this->win_sz = ImGui::GetWindowSize();
    ImGui::End();

    return std::make_pair(win_pos, win_sz);
}

void GameUI::DrawHintTextPanel(ReversiGame &game)
{
    auto &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(win_pos.x, win_pos.y + win_sz.y + 2), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(win_sz.x, 130));
    ImGui::SetNextWindowBgAlpha(win_bg_alpha_);

    ImGui::Begin("Info", nullptr, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoBringToFrontOnFocus|ImGuiWindowFlags_NoCollapse);
    auto cur_win_pos = ImGui::GetWindowPos();
    auto cur_pos = ImGui::GetCursorPos();
    auto draw_list = ImGui::GetWindowDrawList();
    float stone_radius = ImGui::GetFontSize() * 0.49f;

    draw_list->AddCircleFilled(ImVec2(cur_win_pos.x + cur_pos.x + stone_radius, cur_win_pos.y + cur_pos.y + stone_radius), stone_radius, black_col);
    ImGui::SetCursorPosX(cur_pos.x + 2 * stone_radius + 2);
    ImGui::Text(": %d %s", game.count_black_, game.this_game_player_first ? "[Player]" : "");
    cur_pos = ImGui::GetCursorPos();
    draw_list->AddCircleFilled(ImVec2(cur_win_pos.x + cur_pos.x + stone_radius, cur_win_pos.y + cur_pos.y + stone_radius), stone_radius, white_col);
    ImGui::SetCursorPosX(cur_pos.x + 2 * stone_radius + 2);
    ImGui::Text(": %d %s", game.count_white_, game.this_game_player_first ? "" : "[Player]");
    ImGui::Text("%s", game.hint_text_.c_str());
    ImGui::Text("current move: ");
    ImGui::SameLine(0, 0);
    cur_pos = ImGui::GetCursorPos();
    draw_list->AddCircleFilled(ImVec2(cur_win_pos.x + cur_pos.x + stone_radius, cur_win_pos.y + cur_pos.y + stone_radius),
                               stone_radius, game.next_move_stone_ == Stone::WHITE ? white_col : black_col);
    ImGui::End();

    

    ImGui::Text("mouse pos: %.3f, %.3f", io.MousePos.x, io.MousePos.y);
    ImGui::Text("draw pos: %.3f, %.3f", cur_win_pos.x + cur_pos.x + stone_radius, cur_win_pos.x + cur_pos.y + stone_radius);
}
