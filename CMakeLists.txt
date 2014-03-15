project(plasma-shell)


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Script REQUIRED)
find_package(KF5CoreAddons REQUIRED)
find_package(KF5Crash REQUIRED)
find_package(KF5Solid REQUIRED)
find_package(KF5Activities REQUIRED)
find_package(KF5TextEditor) # not part of KF5.0

set(HAVE_KTEXTEDITOR ${KF5TextEditor_FOUND})

configure_file(config-ktexteditor.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ktexteditor.h )


set(scripting_SRC
    scripting/appinterface.cpp
    scripting/applet.cpp
    scripting/containment.cpp
    scripting/configgroup.cpp
    scripting/desktopscriptengine.cpp
    scripting/i18n.cpp
    scripting/panel.cpp
    scripting/rect.cpp
    scripting/scriptengine.cpp
    scripting/widget.cpp
)

set(plasmashell_dbusXML dbus/org.kde.PlasmaShell.xml)
qt5_add_dbus_adaptor(scripting_SRC ${plasmashell_dbusXML} shellcorona.h ShellCorona plasmashelladaptor)

set (plasma_shell_SRCS
    activity.cpp
    main.cpp
    containmentconfigview.cpp
    currentcontainmentactionsmodel.cpp
    desktopview.cpp
    kidenticongenerator.cpp
    panelview.cpp
    panelconfigview.cpp
    panelshadows.cpp
    shellcorona.cpp
    shellmanager.cpp
    osd.cpp
    ${scripting_SRC}
)
if (KF5TextEditor_FOUND)
   set (plasma_shell_SRCS ${plasma_shell_SRCS}
       interactiveconsole.cpp
   )
endif()
add_executable(plasma-shell
    ${plasma_shell_SRCS}
)

target_link_libraries(plasma-shell
 Qt5::Quick
 Qt5::Qml
 Qt5::DBus
 KF5::KIOCore
 KF5::WindowSystem
 KF5::CoreAddons
 KF5::Crash
 KF5::Plasma
 KF5::PlasmaQuick
 Qt5::Script
 KF5::Solid
 KF5::Declarative
 KF5::I18n
 KF5::XmlGui
 KF5::IconThemes
 KF5::Activities
 KF5::GlobalAccel
 KF5::DBusAddons
)
if (TARGET KF5::TextEditor)
 target_link_libraries(plasma-shell KF5::TextEditor)
endif()
target_include_directories(plasma-shell PRIVATE "${CMAKE_BINARY_DIR}")

if(HAVE_X11)
  target_link_libraries(plasma-shell ${X11_LIBRARIES} ${XCB_XCB_LIBRARY} )
  target_link_libraries(plasma-shell Qt5::X11Extras)
endif()

install(TARGETS plasma-shell ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES plasma-shell.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES dbus/org.kde.PlasmaShell.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

add_subdirectory(widgetexplorer)
