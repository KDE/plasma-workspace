include_directories( ${CMAKE_CURRENT_BINARY_DIR}/../../)

set(mouse_engine_SRCS
    mouseengine.cpp
)

if (X11_Xfixes_FOUND)
    set(mouse_engine_SRCS ${mouse_engine_SRCS} cursornotificationhandler.cpp)
endif ()

add_library(plasma_engine_mouse MODULE ${mouse_engine_SRCS})
target_link_libraries(plasma_engine_mouse ${KDE4_KDEUI_LIBS} KF5::Plasma ${X11_LIBRARIES})
if (X11_Xfixes_FOUND)
	target_link_libraries(plasma_engine_mouse ${X11_Xfixes_LIB})
endif ()

install(TARGETS plasma_engine_mouse DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-dataengine-mouse.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

