#pragma once

#include "gui.h"

bool gui_backend_init(Gui* gui, const char* title, uint32_t width, uint32_t height);
void gui_backend_process_events(Gui* gui);
void gui_backend_new_frame(Gui* gui);
void gui_backend_render(Gui* gui);
void gui_backend_shutdown(Gui* gui);
