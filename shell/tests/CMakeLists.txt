
include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(screenpoolmanualtest_SRCS
    screenpooltest.cpp
    ../screenpool.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../screenpool-debug.cpp
    ../outputorderwatcher.cpp
    )
add_executable(screenpoolmanualtest)
qt6_generate_wayland_protocol_client_sources(screenpoolmanualtest FILES
    ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-output-order-v1.xml)

target_sources(screenpoolmanualtest PRIVATE ${screenpoolmanualtest_SRCS})
target_link_libraries(screenpoolmanualtest
            Qt::Test
            Qt::Gui
            Qt::Widgets
            Qt::WaylandClient
            KF6::ConfigCore
            KF6::Service
            KF6::WindowSystem
            KF6::WaylandClient
            Wayland::Client
                        )
if(HAVE_X11)
    target_link_libraries(screenpoolmanualtest XCB::XCB XCB::RANDR)
    target_link_libraries(screenpoolmanualtest Qt::GuiPrivate)
endif()
if(QT_QTOPENGL_FOUND)
    target_link_libraries(screenpoolmanualtest Qt::OpenGL)
endif()


set(setwallpapertest_SRCS
    setwallpapertest.cpp
    )
add_executable(setwallpapertest)

target_sources(setwallpapertest PRIVATE ${setwallpapertest_SRCS})
target_link_libraries(setwallpapertest
            Qt::Test
            Qt::DBus
    )

