#pragma once

#include <dcimgui/dcimgui.h>
#include <SDL3/SDL.h>

typedef struct {
    SDL_Window* window;
    SDL_GLContext gl;
    ImGuiIO* io;
    ImGuiStyle* style;
    struct {
        ImFont* base;
        ImFont* bold;
        ImFont* big;
        ImFont* small;
        ImFont* mono;
    } fonts;
    bool should_close;
} Gui;

Gui* gui_init();
bool gui_should_close(Gui* gui);
void gui_tick(Gui* gui);
void gui_free(Gui* gui);
