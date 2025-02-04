# Directories
BUILD_DIR = build
IMGUI_DIR = lib/vendor/imgui
DCIMGUI_DIR = lib/dcimgui
DEARBINDINGS_DIR = lib/vendor/dear_bindings

# Base config
all: memsed
CC = gcc
LIBS = -lstdc++ -lm
C_FLAGS =
CPP_FLAGS = -std=c++20
UNAME_S = $(shell uname -s)

# OpenGL
ifeq ($(UNAME_S), Linux)
	LIBS += -lGL
endif
ifeq ($(UNAME_S), Darwin)
	LIBS += -framework OpenGL
endif

# SDL
SDL_CONFIG = pkg-config sdl3
CPP_FLAGS += `$(SDL_CONFIG) --cflags`
C_FLAGS += `$(SDL_CONFIG) --cflags`
LIBS += `$(SDL_CONFIG) --libs`

# ImGui bindings
$(DEARBINDINGS_DIR)/venv:
	python3 -m venv $(DEARBINDINGS_DIR)/venv
$(DEARBINDINGS_DIR)/venv/.installed: $(DEARBINDINGS_DIR)/requirements.txt $(DEARBINDINGS_DIR)/venv
	$(DEARBINDINGS_DIR)/venv/bin/pip install -r $(DEARBINDINGS_DIR)/requirements.txt
	cp $(DEARBINDINGS_DIR)/requirements.txt $(DEARBINDINGS_DIR)/venv/.installed
DEARBINDINGS_ARGS = --replace-prefix cIm=Im
DEARBINDINGS_CMD = $(DEARBINDINGS_DIR)/venv/bin/python $(DEARBINDINGS_DIR)/dear_bindings.py $(DEARBINDINGS_ARGS)
.PRECIOUS: $(DCIMGUI_DIR)/backends/dcimgui_impl_%.cpp
$(DCIMGUI_DIR)/backends/dcimgui_impl_%.cpp: $(DEARBINDINGS_DIR)/venv/.installed $(IMGUI_DIR)/backends/imgui_impl_%.h
	@mkdir -p $(DCIMGUI_DIR)/backends
	$(DEARBINDINGS_CMD) \
		--backend \
		-o $(DCIMGUI_DIR)/backends/dcimgui_impl_$* \
		$(IMGUI_DIR)/backends/imgui_impl_$*.h
	rm $(DCIMGUI_DIR)/backends/dcimgui_impl_$**.json
.PRECIOUS: $(DCIMGUI_DIR)/dcim%.cpp
$(DCIMGUI_DIR)/dcim%.cpp: $(DEARBINDINGS_DIR)/venv/.installed $(IMGUI_DIR)/im%.h
	@mkdir -p $(DCIMGUI_DIR)
	$(DEARBINDINGS_CMD) \
		-o $(DCIMGUI_DIR)/dcim$* \
		$(IMGUI_DIR)/im$*.h
	rm $(DCIMGUI_DIR)/dcim$**.json

# ImGui
DCIMGUI_BASE_OBJS = $(BUILD_DIR)/dcimgui.o $(BUILD_DIR)/imgui.o $(BUILD_DIR)/imgui_demo.o $(BUILD_DIR)/imgui_draw.o $(BUILD_DIR)/imgui_tables.o $(BUILD_DIR)/imgui_widgets.o
DCIMGUI_SDL_OBJS = $(BUILD_DIR)/dcimgui_impl_sdl3.o $(BUILD_DIR)/dcimgui_impl_opengl3.o $(BUILD_DIR)/imgui_impl_opengl3.o $(BUILD_DIR)/imgui_impl_sdl3.o
DCIMGUI_OBJS = $(DCIMGUI_BASE_OBJS) $(DCIMGUI_SDL_OBJS)
dcimgui: $(DCIMGUI_OBJS)
CPP_FLAGS += -I$(DCIMGUI_DIR) -I$(IMGUI_DIR) -I$(DCIMGUI_DIR)/backends -I$(IMGUI_DIR)/backends
C_FLAGS += -I$(DCIMGUI_DIR) -I$(IMGUI_DIR) -I$(DCIMGUI_DIR)/backends -I$(IMGUI_DIR)/backends
$(BUILD_DIR)/imgui_impl_%.o: $(IMGUI_DIR)/backends/imgui_impl_%.cpp $(IMGUI_DIR)/imgui.h
	$(CC) $(CPP_FLAGS) -c -o $@ $<
$(BUILD_DIR)/im%.o: $(IMGUI_DIR)/im%.cpp $(IMGUI_DIR)/imgui.h
	$(CC) $(CPP_FLAGS) -c -o $@ $<
$(BUILD_DIR)/dcimgui_impl_%.o: $(DCIMGUI_DIR)/backends/dcimgui_impl_%.cpp $(DCIMGUI_DIR)/dcimgui.h $(IMGUI_DIR)/imgui.h
	$(CC) $(CPP_FLAGS) -c -o $@ $<
$(BUILD_DIR)/dcim%.o: $(DCIMGUI_DIR)/dcim%.cpp $(DCIMGUI_DIR)/dcimgui.h $(IMGUI_DIR)/imgui.h
	$(CC) $(CPP_FLAGS) -c -o $@ $<

# Main targets
run: memsed
	./memsed
memsed: dirs dcimgui $(BUILD_DIR)/main.o
	$(CC) -o memsed $(BUILD_DIR)/main.o $(DCIMGUI_OBJS) $(LIBS)
$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(C_FLAGS) -c -o $@ $<
dirs:
	@mkdir -p $(BUILD_DIR)
clean:
	rm -rf lib/vendor/dear_bindings/venv
	rm -rf lib/dcimgui
	rm -rf $(BUILD_DIR)
	rm -f memsed
