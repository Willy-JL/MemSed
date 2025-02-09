#include "gui/gui.h"

int32_t main(int32_t argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    Gui* gui = gui_init();
    if(gui == NULL) {
        return 1;
    }

    while(!gui_should_close(gui)) {
        gui_tick(gui);
    }

    gui_free(gui);

    return 0;
}
