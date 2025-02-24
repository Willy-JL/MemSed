#include "backend.h"
#include "gui.h"

#include <dcimgui/backends/dcimgui_impl_opengl3.h>
#include <dcimgui/backends/dcimgui_impl_sdl3.h>
#include <dcimgui/dcimgui.h>
#include <SDL3/SDL_opengl.h>

const flt32_t scroll_multiplier = 2.0f;
const flt32_t scroll_smoothing = 8.0f;

bool gui_backend_init(Gui* gui, const char* title, uint32_t width, uint32_t height) {
    // Prefer Wayland when available
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");

    if(!SDL_Init(SDL_INIT_VIDEO)) {
        const char* sdl_init_error = SDL_GetError();
        printf("Error: SDL_Init(): %s\n", sdl_init_error);

        if(strcmp(sdl_init_error, "wayland not available") == 0) {
            SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
            if(!SDL_Init(SDL_INIT_VIDEO)) {
                printf("Error: SDL_Init(): %s\n", SDL_GetError());
                return false;
            }
        } else {
            return false;
        }
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
    const SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                         SDL_WINDOW_HIGH_PIXEL_DENSITY;
    gui->window = SDL_CreateWindow(title, width, height, window_flags);
    if(gui->window == NULL) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return false;
    }
    SDL_SetWindowMinimumSize(gui->window, 720, 400);

    gui->gl = SDL_GL_CreateContext(gui->window);
    SDL_GL_MakeCurrent(gui->window, gui->gl);
    SDL_GL_SetSwapInterval(1);

    const int32_t version = gladLoadGL(SDL_GL_GetProcAddress);
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
    gui->prev_size = (ImVec2){0.0f, 0.0f};
    gui->scroll_energy = (ImVec2){0.0f, 0.0f};

    ImGui_ImplSDL3_InitForOpenGL(gui->window, gui->gl);
    ImGui_ImplOpenGL3_Init();

    return true;
}

void gui_backend_process_events(Gui* gui) {
    SDL_Event event;
    bool requested_close = false;
    while(SDL_PollEvent(&event)) {
        if(event.type == SDL_EVENT_MOUSE_WHEEL &&
           event.window.windowID == SDL_GetWindowID(gui->window)) {
            // Handle wheel events locally to apply smooth scrolling
            event.wheel.x *= scroll_multiplier;
            event.wheel.y *= scroll_multiplier;
            // Immediately stop if direction changes
            if(gui->scroll_energy.x * event.wheel.x < 0.0f) {
                gui->scroll_energy.x = 0.0f;
            }
            if(gui->scroll_energy.y * event.wheel.y < 0.0f) {
                gui->scroll_energy.y = 0.0f;
            }
            gui->scroll_energy.x += event.wheel.x;
            gui->scroll_energy.y += event.wheel.y;
        } else {
            ImGui_ImplSDL3_ProcessEvent(&event);
        }

        // QUIT is sent after WINDOW_CLOSE_REQUESTED for window closing
        // Only QUIT is sent when trying to terminate the process
        // For terminating process, close right away
        // For window close, register request and handle later
        if(event.type == SDL_EVENT_QUIT) {
            if(!requested_close) {
                gui->should_close = true;
            }
        }
        if(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
           event.window.windowID == SDL_GetWindowID(gui->window)) {
            requested_close = true;
        }
    }

    // If window close was requested, save it and handle in draw in case confirmation is needed
    // If requested a second time, just close anyway
    if(requested_close) {
        if(gui->requested_close) {
            gui->should_close = true;
        } else {
            gui->requested_close = true;
        }
    }
}

void gui_backend_new_frame(Gui* gui) {
    UNUSED(gui);

    // Apply smooth scrolling
    ImVec2 scroll_now;
    if(ABS(gui->scroll_energy.x) > 0.01f) {
        scroll_now.x = gui->scroll_energy.x * gui->io->DeltaTime * scroll_smoothing;
        gui->scroll_energy.x -= scroll_now.x;
    } else {
        // Cutoff smoothing when it's basically stopped
        scroll_now.x = 0.0f;
        gui->scroll_energy.x = 0.0f;
    }
    if(ABS(gui->scroll_energy.y) > 0.01f) {
        scroll_now.y = gui->scroll_energy.y * gui->io->DeltaTime * scroll_smoothing;
        gui->scroll_energy.y -= scroll_now.y;
    } else {
        // Cutoff smoothing when it's basically stopped
        scroll_now.y = 0.0f;
        gui->scroll_energy.y = 0.0f;
    }
    gui->io->MouseWheel = scroll_now.y;
    gui->io->MouseWheelH = -scroll_now.x;

    // Hand cursor when hovering buttons and similar
    if(ImGui_IsAnyItemHovered()) {
        ImGui_SetMouseCursor(ImGuiMouseCursor_Hand);
    }

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
