# Directories
OBJ_DIR = obj
IMGUI_DIR = lib/vendor/imgui
DCIMGUI_DIR = lib/dcimgui
DEARBINDINGS_DIR = lib/vendor/dear_bindings

# Base config
all: memsed
CC = gcc
LIBS = -lstdc++ -lm
C_FLAGS =
CPP_FLAGS = -std=c++20
SDL_CONFIG = pkg-config sdl3
UNAME_S = $(shell uname -s)

# OpenGL
ifeq ($(UNAME_S), Linux)
	LIBS += -lGL
endif
ifeq ($(UNAME_S), Darwin)
	LIBS += -framework OpenGL
endif

# SDL
CPP_FLAGS += `$(SDL_CONFIG) --cflags`
C_FLAGS += `$(SDL_CONFIG) --cflags`
LIBS += `$(SDL_CONFIG) --libs`

# ImGui bindings
$(DEARBINDINGS_DIR)/venv:
	python3 -m venv $(DEARBINDINGS_DIR)/venv
	$(DEARBINDINGS_DIR)/venv/bin/pip install -r $(DEARBINDINGS_DIR)/requirements.txt
# Broken: --case-style types=PascalCase --case-style enums=PascalCase
DEARBINDINGS_ARGS = --replace-prefix cIm=Im --case-style fields=snake_case --case-style functions=snake_case --case-style macros=SHOUT_CASE
DEARBINDINGS_CMD = $(DEARBINDINGS_DIR)/venv/bin/python $(DEARBINDINGS_DIR)/dear_bindings.py $(DEARBINDINGS_ARGS)
.PRECIOUS: $(DCIMGUI_DIR)/backends/dcimgui_impl_%.cpp
$(DCIMGUI_DIR)/backends/dcimgui_impl_%.cpp: $(DEARBINDINGS_DIR)/venv
	@mkdir -p $(DCIMGUI_DIR)/backends
	$(DEARBINDINGS_CMD) \
		--backend \
		-o $(DCIMGUI_DIR)/backends/dcimgui_impl_$* \
		$(IMGUI_DIR)/backends/imgui_impl_$*.h
	rm $(DCIMGUI_DIR)/backends/dcimgui_impl_$**.json
.PRECIOUS: $(DCIMGUI_DIR)/dcim%.cpp
$(DCIMGUI_DIR)/dcim%.cpp: $(DEARBINDINGS_DIR)/venv
	@mkdir -p $(DCIMGUI_DIR)
	$(DEARBINDINGS_CMD) \
		-o $(DCIMGUI_DIR)/dcim$* \
		$(IMGUI_DIR)/im$*.h
	rm $(DCIMGUI_DIR)/dcim$**.json

# ImGui
DCIMGUI_BASE_OBJS = $(OBJ_DIR)/dcimgui.o $(OBJ_DIR)/imgui.o $(OBJ_DIR)/imgui_demo.o $(OBJ_DIR)/imgui_draw.o $(OBJ_DIR)/imgui_tables.o $(OBJ_DIR)/imgui_widgets.o
DCIMGUI_SDL_OBJS = $(OBJ_DIR)/dcimgui_impl_sdl3.o $(OBJ_DIR)/dcimgui_impl_opengl3.o $(OBJ_DIR)/imgui_impl_opengl3.o $(OBJ_DIR)/imgui_impl_sdl3.o
DCIMGUI_OBJS = $(DCIMGUI_BASE_OBJS) $(DCIMGUI_SDL_OBJS)
dcimgui: $(DCIMGUI_OBJS)
CPP_FLAGS += -I$(DCIMGUI_DIR) -I$(IMGUI_DIR) -I$(DCIMGUI_DIR)/backends -I$(IMGUI_DIR)/backends
C_FLAGS += -I$(DCIMGUI_DIR) -I$(IMGUI_DIR) -I$(DCIMGUI_DIR)/backends -I$(IMGUI_DIR)/backends
$(OBJ_DIR)/imgui_impl_%.o: $(IMGUI_DIR)/backends/imgui_impl_%.cpp
	$(CC) $(CPP_FLAGS) -c -o $@ $<
$(OBJ_DIR)/im%.o: $(IMGUI_DIR)/im%.cpp
	$(CC) $(CPP_FLAGS) -c -o $@ $<
$(OBJ_DIR)/dcimgui_impl_%.o: $(DCIMGUI_DIR)/backends/dcimgui_impl_%.cpp
	$(CC) $(CPP_FLAGS) -c -o $@ $<
$(OBJ_DIR)/dcim%.o: $(DCIMGUI_DIR)/dcim%.cpp
	$(CC) $(CPP_FLAGS) -c -o $@ $<

# Main targets
run: memsed
	./memsed
memsed: dirs dcimgui $(OBJ_DIR)/main.o
	$(CC) -o memsed $(OBJ_DIR)/main.o $(DCIMGUI_OBJS) $(LIBS)
$(OBJ_DIR)/%.o: src/%.c
	$(CC) $(C_FLAGS) -c -o $@ $<
dirs:
	@mkdir -p $(OBJ_DIR)
clean:
	rm -rf lib/vendor/dear_bindings/venv
	rm -rf lib/dcimgui
	rm -rf $(OBJ_DIR)
	rm -f memsed
