# Build the font generator
add_executable(imgui_font_gen "${IMGUI_DIR}/misc/fonts/binary_to_compressed_c.cpp")

# Find fonts
file(GLOB fonts "${RESOURCES_DIR}/fonts/*.?tf")
set(sources)
set(header "#pragma once\n\n")

# Process fonts
foreach(font_src IN LISTS fonts)
    get_filename_component(font_name "${font_src}" NAME_WE)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" font_symbol "${font_name}")
    string(TOLOWER "${font_symbol}" font_symbol)
    set(font_symbol "font_${font_symbol}")
    set(font_dst "${FONTS_DIR}/${font_symbol}.c")
    # Generate binaries
    file(RELATIVE_PATH font_src_rel "${PROJECT_SOURCE_DIR}" "${font_src}")
    add_custom_command(
        OUTPUT "${font_dst}"
        COMMAND imgui_font_gen -u8 -nostatic "${font_src_rel}" "${font_symbol}" > "${font_dst}"
        DEPENDS imgui_font_gen "${font_src}"
        COMMENT "Generating binary font for ${font_name}"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
    # Append declaration
    string(CONCAT header "${header}" "extern const unsigned int ${font_symbol}_compressed_size;\n")
    string(CONCAT header "${header}" "extern const unsigned char ${font_symbol}_compressed_data[];\n\n")
    # Add sources
    list(APPEND sources "${font_dst}")
endforeach()

# Write generated header
file(WRITE "${FONTS_DIR}/fonts.h" "${header}")
list(APPEND sources "${FONTS_DIR}/fonts.h")

# Generate MaterialDesignIcons mapping
file(GLOB mdi_font "${RESOURCES_DIR}/fonts/materialdesignicons-webfont.*.?tf")
get_filename_component(mdi_name "${mdi_font}" NAME_WE)
add_custom_command(
    OUTPUT "${FONTS_DIR}/mdi.h"
    COMMAND "${Python_EXECUTABLE}" "${FONTS_DIR}/mdi.py" "${mdi_font}" "${FONTS_DIR}/mdi.h"
    DEPENDS "${mdi_font}"
    COMMENT "Generating icon mapping for ${mdi_name}"
)
list(APPEND sources "${FONTS_DIR}/mdi.h")

# Build as static library
add_library(fonts STATIC "${sources}")

# Include paths to use and propagate
target_include_directories(fonts PUBLIC "${FONTS_DIR}")
