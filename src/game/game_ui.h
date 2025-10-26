#ifndef __MY_GAME_UI_H__
#define __MY_GAME_UI_H__

#include "yaml-cpp/yaml.h"

#include "imgui.h"
#include <utility>
class ReversiGame;
class GameUI {
public:
    void LoadConfig(const YAML::Node &node_ui);
    void DumpConfig(YAML::Node &node_ui);

    void DrawBoard(ReversiGame &game_ptr, const ImVec2 &left_top_pos, const ImVec2 &right_btm_pos);
    std::pair<ImVec2, ImVec2> DrawMainPanel(ReversiGame &game);
    void DrawHintTextPanel(ReversiGame &game);

    ImColor background_col;
    ImColor board_fill_col;
    ImColor line_col = ImGui::GetColorU32(ImVec4(1,1,1,1));
    ImColor hint_valid_move_col = ImGui::GetColorU32(ImVec4(0,1,0,0.5f));
    ImColor hint_mouse_pos_col = ImGui::GetColorU32(ImVec4(1,0,0,0.5f));

    float line_interval = 0;
    ImVec2 board_left_top_pos, board_right_btm_pos;
    ImVec2 win_pos, win_sz;

    ImU32 white_col = ImGui::GetColorU32(ImVec4(1,1,1,1));
    ImU32 black_col = ImGui::GetColorU32(ImVec4(0, 0, 0, 1));
private:
    ImVec2 last_cursor_pos_;
    float win_bg_alpha_ = 0.1f;
};

namespace YAML {
    template<>
    struct convert<ImColor> {
        static Node encode(const ImColor &rhs) {
            Node node;
            node = std::vector<float>{rhs.Value.x, rhs.Value.y, rhs.Value.z};
            return node;
        }

        static bool decode(const Node &node, ImColor &rhs) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }
            rhs = ImColor(
                node[0].as<float>(),
                node[1].as<float>(),
                node[2].as<float>());
            return true;
        }
    };
}

#endif