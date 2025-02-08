cmake_minimum_required(VERSION 3.20)
project(MemSed)

# Use clang
set(CMAKE_C_COMPILER "/usr/bin/clang" CACHE STRING "C compiler" FORCE)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Directories
set(MEMSED_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_DIR "${MEMSED_DIR}/src")
set(LIB_DIR "${MEMSED_DIR}/lib")
set(VENDOR_DIR "${LIB_DIR}/vendor")
set(IMGUI_DIR "${VENDOR_DIR}/imgui")
set(DCIMGUI_DIR "${LIB_DIR}/dcimgui")
set(DEARBINDINGS_DIR "${VENDOR_DIR}/dear_bindings")

# Global compiler flags
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
target_compile_options(memsed PRIVATE -std=gnu23)  # Standard
target_compile_options(memsed PRIVATE -Wall -Wextra -Wstrict-prototypes -Wredundant-decls -Wundef)  # Warnings
target_compile_options(memsed PRIVATE -fdata-sections -ffunction-sections -fno-math-errno)  # Optimizations

# ImGui
set(DCIMGUI_BACKENDS sdl3 opengl3)
add_subdirectory("${DCIMGUI_DIR}")
target_link_libraries(memsed PRIVATE dcimgui)

# SDL3
find_package(SDL3 REQUIRED)
target_include_directories(memsed PRIVATE "${SDL3_INCLUDE_DIRS}")
target_link_libraries(memsed PRIVATE "${SDL3_LIBRARIES}")

# OpenGL
find_package(OpenGL REQUIRED)
target_include_directories(memsed PRIVATE "${OPENGL_INCLUDE_DIRS}")
target_link_libraries(memsed PRIVATE "${OPENGL_LIBRARIES}")
