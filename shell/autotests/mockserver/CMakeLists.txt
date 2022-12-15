project(waylandmockservertest)

find_package(WaylandProtocols 1.24)
set_package_properties(WaylandProtocols PROPERTIES TYPE REQUIRED)
find_package(Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_NANOSLEEP" )

set(SharedClientTest_LIB_SRCS
    corecompositor.cpp corecompositor.h
    coreprotocol.cpp coreprotocol.h
    datadevice.cpp datadevice.h
    mockcompositor.cpp mockcompositor.h
    xdgoutputv1.cpp xdgoutputv1.h
    xdgshell.cpp xdgshell.h
    outputorder.cpp outputorder.h
)

add_library(SharedClientTest OBJECT)
if (QT_MAJOR_VERSION EQUAL "5")
    ecm_add_qtwayland_server_protocol(SharedClientTest_LIB_SRCS
        PROTOCOL ${Wayland_DATADIR}/wayland.xml
        BASENAME wayland
    )

    ecm_add_qtwayland_server_protocol(SharedClientTest_LIB_SRCS
        PROTOCOL ${WaylandProtocols_DATADIR}/unstable/xdg-output/xdg-output-unstable-v1.xml
        BASENAME xdg-output-unstable-v1
    )

    ecm_add_qtwayland_server_protocol(SharedClientTest_LIB_SRCS
        PROTOCOL ${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml
        BASENAME xdg-shell
    )
    ecm_add_qtwayland_server_protocol(SharedClientTest_LIB_SRCS
        PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-output-order-v1.xml
        BASENAME kde-output-order-v1
    )
    ecm_add_qtwayland_client_protocol(SharedClientTest_LIB_SRCS
        PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-output-order-v1.xml
        BASENAME kde-output-order-v1
    )

else()
    qt6_generate_wayland_protocol_client_sources(SharedClientTest FILES
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-primary-output-v1.xml
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-output-order-v1.xml
    )
    qt6_generate_wayland_protocol_server_sources(SharedClientTest FILES
        ${Wayland_DATADIR}/wayland.xml
        ${WaylandProtocols_DATADIR}/unstable/xdg-output/xdg-output-unstable-v1.xml
        ${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-primary-output-v1.xml
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-output-order-v1.xml
    )
endif()


target_sources(SharedClientTest PRIVATE ${SharedClientTest_LIB_SRCS})

target_link_libraries(SharedClientTest
    PUBLIC
        Qt::Test
        Qt::Gui
        Qt::WaylandClientPrivate
        Wayland::Server
        Threads::Threads
)

target_include_directories(SharedClientTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
