include_directories( ${CMAKE_CURRENT_BINARY_DIR}/../../)

set(mouse_engine_SRCS
    mouseengine.cpp
    mouseengine.h
)

if (HAVE_X11)
    set(mouse_engine_SRCS ${mouse_engine_SRCS} cursornotificationhandler.cpp)
endif ()

kcoreaddons_add_plugin(plasma_engine_mouse SOURCES ${mouse_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)
target_link_libraries(plasma_engine_mouse
    Qt::Widgets
    KF6::Plasma
    KF6::Plasma5Support
    KF6::WindowSystem
)

if (HAVE_X11)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(plasma_engine_mouse Qt::X11Extras)
    else()
        target_link_libraries(plasma_engine_mouse Qt::GuiPrivate)
    endif()

    target_link_libraries(plasma_engine_mouse X11::X11 X11::Xfixes)
endif ()
