# Directories
SRC_DIR = src
BUILD_DIR = build
IMGUI_DIR = lib/vendor/imgui
DCIMGUI_DIR = lib/dcimgui
DEARBINDINGS_DIR = lib/vendor/dear_bindings

# Base config
all: memsed
CC = clang
LIBS = -lstdc++ -lm
C_FLAGS = -Ilib
CPP_FLAGS = -Ilib -std=c++20
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
DEARBINDINGS_ARGS = --replace-prefix cIm=Im --replace-prefix CIM=IM
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
IMGUI_SRCS = $(wildcard $(IMGUI_DIR)/*.cpp)
IMGUI_BACKENDS = impl_sdl3 impl_opengl3
IMGUI_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(IMGUI_SRCS))
IMGUI_BACKENDS_OBJS = $(patsubst %,$(BUILD_DIR)/$(IMGUI_DIR)/backends/imgui_%.o,$(IMGUI_BACKENDS))
DCIMGUI_BACKENDS_OBJS = $(patsubst %,$(BUILD_DIR)/$(DCIMGUI_DIR)/backends/dcimgui_%.o,$(IMGUI_BACKENDS))
DCIMGUI_OBJS = $(BUILD_DIR)/$(DCIMGUI_DIR)/dcimgui.o $(IMGUI_OBJS) $(DCIMGUI_BACKENDS_OBJS) $(IMGUI_BACKENDS_OBJS)
dcimgui: $(DCIMGUI_OBJS)
CPP_FLAGS += -I$(DCIMGUI_DIR) -I$(IMGUI_DIR) -I$(DCIMGUI_DIR)/backends -I$(IMGUI_DIR)/backends
C_FLAGS += -I$(DCIMGUI_DIR) -I$(IMGUI_DIR) -I$(DCIMGUI_DIR)/backends -I$(IMGUI_DIR)/backends
$(BUILD_DIR)/$(IMGUI_DIR)/%.o: $(IMGUI_DIR)/%.cpp $(IMGUI_DIR)/imgui.h
	@mkdir -p $(@D)
	$(CC) $(CPP_FLAGS) -c -o $@ $<
$(BUILD_DIR)/$(DCIMGUI_DIR)/%.o: $(DCIMGUI_DIR)/%.cpp $(IMGUI_DIR)/imgui.h
	@mkdir -p $(@D)
	$(CC) $(CPP_FLAGS) -c -o $@ $<

# Main targets
run: memsed
	./memsed
MEMSED_SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/**/*.c)
MEMSED_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(MEMSED_SRCS))
memsed: dcimgui $(MEMSED_OBJS)
	@mkdir -p $(@D)
	$(CC) -o memsed $(MEMSED_OBJS) $(DCIMGUI_OBJS) $(LIBS)
$(BUILD_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -c -o $@ $<
clean:
	rm -rf lib/vendor/dear_bindings/venv
	rm -rf lib/dcimgui
	rm -rf $(BUILD_DIR)
	rm -f memsed
