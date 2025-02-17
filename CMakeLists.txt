cmake_minimum_required(VERSION 3.20)
project(MemSed)

# Directories
set(MEMSED_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(BUILD_DIR "${MEMSED_DIR}/build")
set(SRC_DIR "${MEMSED_DIR}/src")
set(LIB_DIR "${MEMSED_DIR}/lib")
set(FONTS_DIR "${LIB_DIR}/fonts")
set(VENDOR_DIR "${LIB_DIR}/vendor")
set(RESOURCES_DIR "${MEMSED_DIR}/resources")
set(VENV_DIR "${CMAKE_CURRENT_BINARY_DIR}/venv")
set(GLAD_DIR "${LIB_DIR}/glad")
set(GLAD_GEN "${VENDOR_DIR}/glad")
set(SDL3_DIR "${VENDOR_DIR}/sdl3")
set(IMGUI_DIR "${VENDOR_DIR}/imgui")
set(DCIMGUI_DIR "${LIB_DIR}/dcimgui")
set(DEARBINDINGS_DIR "${VENDOR_DIR}/dear_bindings")

# Export Clang DB
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
file(CREATE_LINK "${CMAKE_CURRENT_BINARY_DIR}" "${BUILD_DIR}/current" SYMBOLIC)

# Global compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-O0)  # Better debugging experience
endif()
add_compile_options(-Werror)  # Fail on any warning

# Global includes
# Allows #include <dcimgui/dcimgui.h> instead of just #include <dcimgui.h>
include_directories("${LIB_DIR}" "${VENDOR_DIR}")

# Main sources
# Globbing is discouraged but fine when keeping in mind its caveat:
# Adding new files weon't be detected, need to run cmake again to detect them
# You can also `touch CMakeLists.txt`, or save it without making changes
file(GLOB_RECURSE sources "${SRC_DIR}/*.c" "${SRC_DIR}/*.h")

# Main executable
add_executable(memsed "${sources}")

# Compiler flags
target_compile_options(memsed PRIVATE -std=gnu2x)  # Standard
target_compile_options(memsed PRIVATE -Wall -Wextra -Wstrict-prototypes -Wredundant-decls -Wundef)  # Warnings
target_compile_options(memsed PRIVATE -fdata-sections -ffunction-sections -fno-math-errno)  # Optimizations

# Python (needed for GLAD and dear_bindings)
find_package(Python COMPONENTS Interpreter REQUIRED)
execute_process(COMMAND "${Python_EXECUTABLE}" -m venv "${VENV_DIR}")
set(Python_EXECUTABLE "${VENV_DIR}/bin/python")
execute_process(COMMAND "${Python_EXECUTABLE}" -m pip install -U -r "${MEMSED_DIR}/requirements.txt")
configure_file("${MEMSED_DIR}/requirements.txt" "${VENV_DIR}/.installed")

# OpenGL (via GLAD loader)
add_subdirectory("${GLAD_GEN}/cmake")
glad_add_library(glad STATIC EXCLUDE_FROM_ALL MERGE REPRODUCIBLE API gl:core=3.0 LOCATION "${GLAD_DIR}")
target_link_libraries(memsed PRIVATE glad)

# SDL3
set(SDL_STATIC ON)
add_subdirectory("${SDL3_DIR}" EXCLUDE_FROM_ALL)
target_link_libraries(memsed PRIVATE SDL3-static)

# ImGui (via dcimgui bindings)
set(DCIMGUI_BACKENDS sdl3 opengl3)
add_subdirectory("${DCIMGUI_DIR}")
target_link_libraries(dcimgui PRIVATE SDL3-static)
target_link_libraries(dcimgui PRIVATE glad)
target_link_libraries(memsed PRIVATE dcimgui)

# Fonts (generate compressed C sources)
add_subdirectory("${FONTS_DIR}")
target_link_libraries(memsed PRIVATE fonts)

# Build as (almost) static executable
# Adding -static causes a segfault in SDL_CreateWindow()
target_link_libraries(memsed PRIVATE -static-libgcc -static-libstdc++)

# Add run targets
add_custom_target(run
    COMMAND sudo ./memsed
    DEPENDS memsed
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
add_custom_target(gdb
    COMMAND sudo gdb -q ./memsed
    DEPENDS memsed
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
