#include "window.h"

void gui_window_draw(Gui* gui) {
    ImGui_SetNextWindowPos((ImVec2){0.0f, 0.0f}, ImGuiCond_Once);
    int32_t width, height;
    SDL_GetWindowSize(gui->window, &width, &height);
    ImVec2 size = {width, height};
    if(size.x != gui->prev_size.x || size.y != gui->prev_size.y) {
        ImGui_SetNextWindowSize(size, ImGuiCond_Always);
    }

    ImGui_PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui_Begin(
        "MemSed",
        NULL,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);
    ImGui_PopStyleVar();

    ImGui_Text(mdi_rocket_launch " FPS: %.1f", gui->io->Framerate);

    ImGui_End();
}
