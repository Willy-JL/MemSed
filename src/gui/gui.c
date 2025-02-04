#include "gui.h"
#include "backend.h"
#include "window.h"

#include <stdlib.h>

Gui* gui_init() {
    Gui* gui = malloc(sizeof(Gui));
    gui->should_close = false;

    if(!gui_backend_init(gui, "MemSed", 1000, 1000)) {
        free(gui);
        return NULL;
    }

    return gui;
}

bool gui_should_close(Gui* gui) {
    return gui->should_close;
}

void gui_tick(Gui* gui) {
    gui_backend_process_events(gui);
    if(gui->should_close) {
        return;
    }
    gui_backend_new_frame(gui);
    gui_window_draw(gui);
    gui_backend_render(gui);
}

void gui_free(Gui* gui) {
    gui_backend_shutdown(gui);
    free(gui);
}
