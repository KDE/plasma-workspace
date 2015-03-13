include_directories( ${CMAKE_CURRENT_BINARY_DIR}/../../)

set(mouse_engine_SRCS
    mouseengine.cpp
)

if (X11_Xfixes_FOUND)
    set(mouse_engine_SRCS ${mouse_engine_SRCS} cursornotificationhandler.cpp)
endif ()

add_library(plasma_engine_mouse MODULE ${mouse_engine_SRCS})
target_link_libraries(plasma_engine_mouse
    Qt5::Widgets
    Qt5::X11Extras
    KF5::Plasma
    KF5::WindowSystem
    ${X11_LIBRARIES}
)

if (X11_Xfixes_FOUND)
	target_link_libraries(plasma_engine_mouse ${X11_Xfixes_LIB})
endif ()

kcoreaddons_desktop_to_json(plasma_engine_mouse plasma-dataengine-mouse.desktop)

install(TARGETS plasma_engine_mouse DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-mouse.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )

