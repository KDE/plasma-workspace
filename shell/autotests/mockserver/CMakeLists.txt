find_package(WaylandProtocols 1.24)
set_package_properties(WaylandProtocols PROPERTIES TYPE REQUIRED)

add_executable(qtwaylandscanner_kde qtwaylandscanner.cpp)
target_link_libraries(qtwaylandscanner_kde Qt::Core)

function(ecm_add_qtwayland_server_protocol_kde out_var)
    # Parse arguments
    set(oneValueArgs PROTOCOL BASENAME PREFIX)
    cmake_parse_arguments(ARGS "" "${oneValueArgs}" "" ${ARGN})

    if(ARGS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to ecm_add_qtwayland_server_protocol_kde(): \"${ARGS_UNPARSED_ARGUMENTS}\"")
    endif()

    set(_prefix "${ARGS_PREFIX}")


    find_package(WaylandScanner REQUIRED QUIET)
    #Don't generate/compile client files or it will cause linker errors
    #ecm_add_wayland_server_protocol(${out_var}
                                    #PROTOCOL ${ARGS_PROTOCOL}
                                    #BASENAME ${ARGS_BASENAME})

    get_filename_component(_infile ${ARGS_PROTOCOL} ABSOLUTE)
    set(_header "${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-${ARGS_BASENAME}.h")
    set(_code "${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-${ARGS_BASENAME}.cpp")

    set_source_files_properties(${_header} ${_code} GENERATED)

    add_custom_command(OUTPUT "${_header}"
        COMMAND qtwaylandscanner_kde server-header ${_infile} "" ${_prefix} > ${_header}
        DEPENDS ${_infile} qtwaylandscanner_kde VERBATIM)

    add_custom_command(OUTPUT "${_code}"
        COMMAND qtwaylandscanner_kde server-code ${_infile} "" ${_prefix} > ${_code}
        DEPENDS ${_infile} ${_header} qtwaylandscanner_kde VERBATIM)

    set_property(SOURCE ${_header} ${_code} PROPERTY SKIP_AUTOMOC ON)

    list(APPEND ${out_var} "${_code}")
    set(${out_var} ${${out_var}} PARENT_SCOPE)
endfunction()

set(SharedClientTest_LIB_SRCS
    corecompositor.cpp corecompositor.h
    coreprotocol.cpp coreprotocol.h
    datadevice.cpp datadevice.h
    mockcompositor.cpp mockcompositor.h
    textinput.cpp textinput.h
    xdgoutputv1.cpp xdgoutputv1.h
    xdgshell.cpp xdgshell.h
    primaryoutput.cpp primaryoutput.h
)

ecm_add_qtwayland_server_protocol_kde(SharedClientTest_LIB_SRCS
    PROTOCOL ${Wayland_DATADIR}/wayland.xml
    BASENAME wayland
)

ecm_add_qtwayland_server_protocol_kde(SharedClientTest_LIB_SRCS
    PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/text-input-unstable-v2.xml
    BASENAME text-input-unstable-v2
)

ecm_add_qtwayland_server_protocol_kde(SharedClientTest_LIB_SRCS
    PROTOCOL ${WaylandProtocols_DATADIR}/unstable/xdg-output/xdg-output-unstable-v1.xml
    BASENAME xdg-output-unstable-v1
)

ecm_add_qtwayland_server_protocol_kde(SharedClientTest_LIB_SRCS
    PROTOCOL ${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml
    BASENAME xdg-shell
)

ecm_add_qtwayland_server_protocol_kde(SharedClientTest_LIB_SRCS
    PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-primary-output-v1.xml
    BASENAME kde-primary-output-v1
)

add_library(SharedClientTest OBJECT ${SharedClientTest_LIB_SRCS})

target_link_libraries(SharedClientTest
    PUBLIC
        Qt::Gui
        Qt::WaylandClientPrivate
        Wayland::Server
        Threads::Threads # special case
)

target_include_directories(SharedClientTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
