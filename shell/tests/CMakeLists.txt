
include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(screenpoolmanualtest_SRCS
    screenpooltest.cpp
    ../screenpool.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../screenpool-debug.cpp
    ../outputorderwatcher.cpp
    )
add_executable(screenpoolmanualtest)
if (QT_MAJOR_VERSION EQUAL "5")
    ecm_add_qtwayland_client_protocol(screenpoolmanualtest_SRCS
        PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-output-order-v1.xml
        BASENAME kde-output-order-v1
    )
else()
    qt6_generate_wayland_protocol_client_sources(screenpoolmanualtest FILES
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-output-order-v1.xml)
endif()

target_sources(screenpoolmanualtest PRIVATE ${screenpoolmanualtest_SRCS})
target_link_libraries(screenpoolmanualtest
            Qt::Test
            Qt::Gui
            Qt::Widgets
            Qt::WaylandClient
            KF5::ConfigCore
            KF5::Service
            KF5::WindowSystem
            KF5::WaylandClient
            Wayland::Client
                        )
if(HAVE_X11)
    target_link_libraries(screenpoolmanualtest XCB::XCB XCB::RANDR)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(screenpoolmanualtest Qt::X11Extras)
    else()
        target_link_libraries(screenpoolmanualtest Qt::GuiPrivate)
    endif()
endif()
if(QT_QTOPENGL_FOUND)
    target_link_libraries(screenpoolmanualtest Qt::OpenGL)
endif()

