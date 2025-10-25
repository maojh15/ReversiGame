#ifndef __REVERSI_GAME_H__
#define __REVERSI_GAME_H__

#include "imgui.h"
#include "SDL.h"

class ReversiGame {
public:
    static ReversiGame &GetInstance() {
        static ReversiGame instance;
        return instance;
    }

    Uint64 last_tick = 0;
    void MainLoop() {
        Uint64 tick = SDL_GetTicks64();
        float fps = 1000.0 / (tick - last_tick);
        last_tick = tick;
        auto &io = ImGui::GetIO();
        ImGui::Begin("test");
        ImGui::Text("fps: %.3f", fps);
        ImGui::Text("Test reversigame render");
        ImGui::End();
    }

private:
    ReversiGame() {}

};

#endif