#include "reversi_game.h"

#include "imgui_idle_optimizer.h"
#include "impl_sdl2.h"

#include <iostream>

// Main code
int main(int, char**)
{
    SDL_Window *window;
    SDL_GLContext gl_context;
    int retval = SetupSDL2(window, gl_context, 800, 600, "Reversi Game");
    if (retval != 0) {
        return retval;
    }

    LoadFonts();

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ReversiGame &game = ReversiGame::GetInstance();

    ImGuiIdleOptimizer idle_optim(1.0, 5.0f, 60.0f);
    double ms_per_frame = 1000.0 / 60;
    auto &io = ImGui::GetIO();
    // Main loop
    bool done = false;
    Uint64 last_time_ms = SDL_GetTicks64();
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        idle_optim.beginFrame();

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        bool active_flag = false;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            active_flag = active_flag || io.WantCaptureMouse || io.WantCaptureKeyboard;
            if (event.type == SDL_QUIT) {
                done = true;
                active_flag = true;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
                done = true;
                active_flag = true;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q) {
                done = true;
                active_flag = true;
            }
        }
        if (active_flag) {
            idle_optim.onUserInput();
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 9);

        game.MainLoop();

        ImGui::PopStyleVar(1);

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
    
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    CleanUpSDL2(window, gl_context);
    return 0;
}
