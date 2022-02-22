
include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(screenpoolmanualtest_SRCS
    screenpooltest.cpp
    ../screenpool.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../screenpool-debug.cpp
    ../primaryoutputwatcher.cpp
    )
ecm_add_qtwayland_client_protocol(screenpoolmanualtest_SRCS
        PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-primary-output-v1.xml
        BASENAME kde-primary-output-v1
    )
add_executable(screenpoolmanualtest ${screenpoolmanualtest_SRCS})
target_link_libraries(screenpoolmanualtest
            Qt::Test
            Qt::Gui
            KF5::Service
            KF5::WindowSystem
            KF5::WaylandClient
            Wayland::Client
                        )
if(HAVE_X11)
    target_link_libraries(screenpoolmanualtest XCB::XCB XCB::RANDR)
    target_link_libraries(screenpoolmanualtest Qt::X11Extras)
endif()
if(QT_QTOPENGL_FOUND)
    target_link_libraries(screenpoolmanualtest Qt::OpenGL)
endif()

