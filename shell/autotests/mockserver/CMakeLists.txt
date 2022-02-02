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
    ecm_add_wayland_server_protocol(${out_var}
                                    PROTOCOL ${ARGS_PROTOCOL}
                                    BASENAME ${ARGS_BASENAME})

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

#qt_manual_moc(moc_files
    #mockcompositor.h
    #coreprotocol.h
    #corecompositor.h
    #datadevice.h
    #fullscreenshellv1.h
    #iviapplication.h
    #textinput.h
    #qttextinput.h
    #xdgoutputv1.h
    #xdgshell.h
#)

set(SharedClientTest_LIB_SRCS
    corecompositor.cpp corecompositor.h
    coreprotocol.cpp coreprotocol.h
    datadevice.cpp datadevice.h
    # fullscreenshellv1.cpp fullscreenshellv1.h
    mockcompositor.cpp mockcompositor.h
    textinput.cpp textinput.h
    #  qttextinput.cpp qttextinput.h
    xdgoutputv1.cpp xdgoutputv1.h
    xdgshell.cpp xdgshell.h
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

add_library(SharedClientTest OBJECT ${SharedClientTest_LIB_SRCS})

#[[
qt6_generate_wayland_protocol_server_sources(SharedClientTest
    FILES
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/fullscreen-shell-unstable-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/ivi-application.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/wp-primary-selection-unstable-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/tablet-unstable-v2.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/text-input-unstable-v2.xml
        ${PROJECT_SOURCE_DIR}/src/extensions/qt-text-input-method-unstable-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/wayland.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/xdg-decoration-unstable-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/xdg-output-unstable-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/xdg-shell.xml
)]]

target_link_libraries(SharedClientTest
    PUBLIC
        Qt::Gui
      #  Qt::OpenGL
        Qt::WaylandClientPrivate
        Wayland::Server
        Threads::Threads # special case
)

target_include_directories(SharedClientTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
