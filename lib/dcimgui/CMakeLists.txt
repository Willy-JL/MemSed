# Condigure bindings generation
set(DEARBINDINGS_ARG --replace-prefix cIm=Im --replace-prefix CIM=IM --imconfig-path "${DCIMGUI_DIR}/dcimconfig.h" --emit-combined-json-metadata)
set(DEARBINDINGS_CMD "${Python_EXECUTABLE}" "${DEARBINDINGS_DIR}/dear_bindings.py" "${DEARBINDINGS_ARG}")

# Library
set(dcimgui_sources "${DCIMGUI_DIR}/dcimgui.cpp" "${DCIMGUI_DIR}/dcimgui.h")
set(dcimgui_internal_sources "${DCIMGUI_DIR}/dcimgui_internal.cpp" "${DCIMGUI_DIR}/dcimgui_internal.h")
# Generate bindings
add_custom_command(
    OUTPUT ${dcimgui_sources}
    COMMAND ${DEARBINDINGS_CMD} -o "${DCIMGUI_DIR}/dcimgui" "${IMGUI_DIR}/imgui.h"
    COMMAND "${CMAKE_COMMAND}" -E rm "${DCIMGUI_DIR}/dcimgui.json"
    DEPENDS "${IMGUI_DIR}/imgui.h"
    COMMENT "Generating dcimgui bindings for imgui.h"
    USES_TERMINAL
)
add_custom_command(
    OUTPUT ${dcimgui_internal_sources}
    COMMAND ${DEARBINDINGS_CMD} -o "${DCIMGUI_DIR}/dcimgui_internal" --include "${IMGUI_DIR}/imgui.h" "${IMGUI_DIR}/imgui_internal.h"
    COMMAND "${CMAKE_COMMAND}" -E rm "${DCIMGUI_DIR}/dcimgui_internal.json"
    DEPENDS "${IMGUI_DIR}/imgui_internal.h" "${IMGUI_DIR}/imgui.h"
    COMMENT "Generating dcimgui bindings for imgui_internal.h"
    USES_TERMINAL
)
# Add sources
file(GLOB imgui_sources "${IMGUI_DIR}/*.cpp" "${IMGUI_DIR}/*.h")
set(sources "${dcimgui_sources}" "${dcimgui_internal_sources}" "${imgui_sources}")

# Backends
foreach(backend IN LISTS DCIMGUI_BACKENDS)
    set(backend_dst "${DCIMGUI_DIR}/backends/dcimgui_impl_${backend}")
    set(backend_src "${IMGUI_DIR}/backends/imgui_impl_${backend}")
    # Generate bindings
    add_custom_command(
        OUTPUT "${backend_dst}.cpp" "${backend_dst}.h"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DCIMGUI_DIR}/backends"
        COMMAND ${DEARBINDINGS_CMD} --backend -o "${backend_dst}" "${backend_src}.h"
        COMMAND "${CMAKE_COMMAND}" -E rm "${backend_dst}.json"
        DEPENDS "${backend_src}.h" "${IMGUI_DIR}/imgui.h"
        COMMENT "Generating dcimgui bindings for backends/imgui_impl_${backend}.h"
        USES_TERMINAL
    )
    # Add sources
    list(APPEND sources "${backend_dst}.cpp" "${backend_dst}.h")
    list(APPEND sources "${backend_src}.cpp" "${backend_src}.h")
endforeach()

# Build as static library
add_library(dcimgui STATIC "${sources}")

# Include paths to use and propagate
target_include_directories(dcimgui PUBLIC "${DCIMGUI_DIR}" "${DCIMGUI_DIR}/backends")
target_include_directories(dcimgui PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends")

# Custom imconfig.h
target_compile_definitions(dcimgui PUBLIC IMGUI_USER_CONFIG="dcimconfig.h")
