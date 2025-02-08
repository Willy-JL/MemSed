# Main sources
file(GLOB dcimgui_sources "${DCIMGUI_DIR}/*.cpp" "${DCIMGUI_DIR}/*.h")
file(GLOB imgui_sources "${IMGUI_DIR}/*.cpp" "${IMGUI_DIR}/*.h")
set(sources "${dcimgui_sources}" "${imgui_sources}")

# Backends sources
foreach(backend IN LISTS DCIMGUI_BACKENDS)
    list(APPEND sources "${DCIMGUI_DIR}/backends/dcimgui_impl_${backend}.cpp")
    list(APPEND sources "${DCIMGUI_DIR}/backends/dcimgui_impl_${backend}.h")
    list(APPEND sources "${IMGUI_DIR}/backends/imgui_impl_${backend}.cpp")
    list(APPEND sources "${IMGUI_DIR}/backends/imgui_impl_${backend}.h")
endforeach()

# Build as static library
add_library(dcimgui STATIC "${sources}")

# Include paths to use and propagate
target_include_directories(dcimgui PUBLIC "${DCIMGUI_DIR}" "${DCIMGUI_DIR}/backends")
target_include_directories(dcimgui PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends")

# Disable unavoidable warnings
target_compile_options(dcimgui PRIVATE -Wno-macro-redefined)

# Custom imconfig.h
target_compile_definitions(dcimgui PUBLIC IMGUI_USER_CONFIG="dcimconfig.h")
