#ifndef __IDLE_TOOL_H__
#define __IDLE_TOOL_H__

#include "SDL2/SDL.h"

#include <iostream>

struct IdleTool {
    IdleTool(bool enable_idle = true, Uint64 idle_time_threshold_ms = 1.0, float idle_fps = 9.0f) 
        : enable_idle{enable_idle}, idle_threshold_ms{idle_time_threshold_ms} {
        idle_wait_time_ms = 1000.0 / idle_fps;
    }

    void IdleSleep() {
        if (!enable_idle) return;
        if (accum_idle_time_ms >= idle_threshold_ms) {
            std::cout << "Idle sleep for " << idle_wait_time_ms << " ms!" << std::endl;
            SDL_Delay(idle_wait_time_ms);
        } else {
            Uint32 current_time_ms = SDL_GetTicks64();
            accum_idle_time_ms += (current_time_ms - last_time_ms);
            last_time_ms = current_time_ms; 
        }

    }

    void ResetIdleTime() {
        std::cout << SDL_GetTicks64() << "Reset idle time!" << std::endl;
        accum_idle_time_ms = 0;
        last_time_ms = SDL_GetTicks64();
    }

    Uint64 accum_idle_time_ms = 0;
    Uint32 idle_wait_time_ms = 0;
    Uint64 idle_threshold_ms = 0;
    bool enable_idle = true;

private:
    Uint64 last_time_ms = 0;
};

#endif