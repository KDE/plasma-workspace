include_directories( ${CMAKE_CURRENT_BINARY_DIR}/../../)

set(mouse_engine_SRCS
    mouseengine.cpp
)

if (X11_Xfixes_FOUND)
    set(mouse_engine_SRCS ${mouse_engine_SRCS} cursornotificationhandler.cpp)
endif ()

kcoreaddons_add_plugin(plasma_engine_mouse SOURCES ${mouse_engine_SRCS} INSTALL_NAMESPACE plasma/dataengine)
target_link_libraries(plasma_engine_mouse
    Qt::Widgets
    Qt::X11Extras
    KF5::Plasma
    KF5::WindowSystem
    X11::X11
)

if (X11_Xfixes_FOUND)
	target_link_libraries(plasma_engine_mouse X11::Xfixes)
endif ()

kcoreaddons_desktop_to_json(plasma_engine_mouse plasma-dataengine-mouse.desktop)
