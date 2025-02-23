#include <glad/gl.h>

// 32-bit characters for drawing
#define IMGUI_USE_WCHAR32 1

// Don't provide barebones OpenGL definitions from ImGui
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM 1

// Remove imgui.h defines before dcimgui.h re-defines them
#undef IMGUI_CHECKVERSION
#undef IM_ALLOC
#undef IM_FREE
