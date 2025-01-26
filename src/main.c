// Dear ImGui: standalone example application for SDL3 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_sdl3_opengl3/ folder**
// See imgui_impl_sdl3.cpp for details.

#include "dcimgui.h"
#include "dcimgui_impl_sdl3.h"
#include "dcimgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

// Main code
int main(int argc, char** argv)
{
    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+OpenGL example", 1280, 720, window_flags);
    if (window == NULL)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(0); // Enable vsync

    // Setup Dear ImGui context
    // IMGUI_CHECKVERSION();
    imgui_create_context(NULL);
    ImGuiIO* io = imgui_get_io(); (void)io;
    io->config_flags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    imgui_style_colors_dark(NULL);
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    imgui_impl_sdl3_init_for_opengl(window, gl_context);
    imgui_impl_opengl3_init();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = { 0.45f, 0.55f, 0.60f, 1.00f };

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            imgui_impl_sdl3_process_event(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        imgui_impl_opengl3_new_frame();
        imgui_impl_sdl3_new_frame();
        imgui_new_frame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            imgui_show_demo_window(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            imgui_begin("Hello, world!", NULL, ImGuiWindowFlags_None);                          // Create a window called "Hello, world!" and append into it.

            imgui_text("This is some useful text.");               // Display some text (you can use a format strings too)
            imgui_checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            imgui_checkbox("Another Window", &show_another_window);

            imgui_slider_float("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            imgui_color_edit3("clear color", (float*)&clear_color, ImGuiColorEditFlags_None); // Edit 3 floats representing a color

            if (imgui_button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            imgui_same_line();
            imgui_text("counter = %d", counter);

            imgui_text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->framerate, io->framerate);
            imgui_end();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            imgui_begin("Another Window", &show_another_window, ImGuiWindowFlags_None);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            imgui_text("Hello from another window!");
            if (imgui_button("Close Me"))
                show_another_window = false;
            imgui_end();
        }

        // Rendering
        imgui_render();
        glViewport(0, 0, (int)io->display_size.x, (int)io->display_size.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        imgui_impl_opengl3_render_draw_data(imgui_get_draw_data());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    imgui_impl_opengl3_shutdown();
    imgui_impl_sdl3_shutdown();
    imgui_destroy_context(NULL);

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
