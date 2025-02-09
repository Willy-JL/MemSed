#include "backend.h"
#include "gui.h"

#include <dcimgui/backends/dcimgui_impl_opengl3.h>
#include <dcimgui/backends/dcimgui_impl_sdl3.h>
#include <dcimgui/dcimgui.h>
#include <SDL3/SDL_opengl.h>

void gui_backend_update_cursor(Gui* gui) {
    if(ImGui_IsAnyItemHovered()) {
        ImGui_SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    ImGuiMouseCursor cursor = ImGui_GetMouseCursor();

    if(cursor != gui->prev_cursor) {
        SDL_SystemCursor system_cursor;
        switch(cursor) {
        case ImGuiMouseCursor_None:
        case ImGuiMouseCursor_Arrow:
            system_cursor = SDL_SYSTEM_CURSOR_DEFAULT;
            break;
        case ImGuiMouseCursor_TextInput:
            system_cursor = SDL_SYSTEM_CURSOR_TEXT;
            break;
        case ImGuiMouseCursor_ResizeAll:
            system_cursor = SDL_SYSTEM_CURSOR_MOVE;
            break;
        case ImGuiMouseCursor_ResizeNS:
            system_cursor = SDL_SYSTEM_CURSOR_NS_RESIZE;
            break;
        case ImGuiMouseCursor_ResizeEW:
            system_cursor = SDL_SYSTEM_CURSOR_EW_RESIZE;
            break;
        case ImGuiMouseCursor_ResizeNESW:
            system_cursor = SDL_SYSTEM_CURSOR_NESW_RESIZE;
            break;
        case ImGuiMouseCursor_ResizeNWSE:
            system_cursor = SDL_SYSTEM_CURSOR_NWSE_RESIZE;
            break;
        case ImGuiMouseCursor_Hand:
            system_cursor = SDL_SYSTEM_CURSOR_POINTER;
            break;
        case ImGuiMouseCursor_NotAllowed:
            system_cursor = SDL_SYSTEM_CURSOR_NOT_ALLOWED;
            break;
        }

        SDL_Cursor* sdl_cursor = SDL_CreateSystemCursor(system_cursor);
        SDL_SetCursor(sdl_cursor);
        SDL_DestroyCursor(sdl_cursor);

        gui->prev_cursor = cursor;
    }
}

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

    int32_t version = gladLoadGL(SDL_GL_GetProcAddress);
    if(version == 0) {
        printf("Error: gladLoadGL(): Failed to initialize OpenGL context\n");
        SDL_GL_DestroyContext(gui->gl);
        SDL_DestroyWindow(gui->window);
        SDL_Quit();
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui_CreateContext(NULL);
    gui->io = ImGui_GetIO();
    gui->style = ImGui_GetStyle();
    gui->prev_cursor = ImGuiMouseCursor_None;
    gui->prev_size = (ImVec2){0, 0};

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
    gui_backend_update_cursor(gui);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui_NewFrame();
}

void gui_backend_render(Gui* gui) {
    ImGui_Render();
    glViewport(0, 0, (int32_t)gui->io->DisplaySize.x, (int32_t)gui->io->DisplaySize.y);
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
