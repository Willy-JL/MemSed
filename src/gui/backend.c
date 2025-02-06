#include "gui.h"

#include <dcimgui/backends/dcimgui_impl_opengl3.h>
#include <dcimgui/backends/dcimgui_impl_sdl3.h>
#include <SDL3/SDL_opengl.h>
#include <stdio.h>

bool gui_backend_init(Gui* gui, const char* title, uint32_t width, uint32_t height) {
    // Prefer Wayland when available
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");

    if(!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return false;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                   SDL_WINDOW_HIGH_PIXEL_DENSITY;
    gui->window = SDL_CreateWindow(title, width, height, window_flags);
    if(gui->window == NULL) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return false;
    }

    gui->gl = SDL_GL_CreateContext(gui->window);
    SDL_GL_MakeCurrent(gui->window, gui->gl);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui_CreateContext(NULL);
    gui->io = ImGui_GetIO();
    gui->io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_StyleColorsDark(NULL);

    ImGui_ImplSDL3_InitForOpenGL(gui->window, gui->gl);
    ImGui_ImplOpenGL3_Init();

    return true;
}

void gui_backend_process_events(Gui* gui) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if(event.type == SDL_EVENT_QUIT) {
            gui->should_close = true;
        }
        if(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
           event.window.windowID == SDL_GetWindowID(gui->window)) {
            gui->should_close = true;
        }
    }
}

void gui_backend_new_frame(Gui* gui) {
    (void)gui;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui_NewFrame();
}

void gui_backend_render(Gui* gui) {
    ImGui_Render();
    glViewport(0, 0, (int32_t)gui->io->DisplaySize.x, (int32_t)gui->io->DisplaySize.y);
    // glClearColor(0.0, 0.0, 0.0, 1.0);
    // glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui_GetDrawData());
    SDL_GL_SwapWindow(gui->window);
}

void gui_backend_shutdown(Gui* gui) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui_DestroyContext(NULL);

    SDL_GL_DestroyContext(gui->gl);
    SDL_DestroyWindow(gui->window);
    SDL_Quit();
}
