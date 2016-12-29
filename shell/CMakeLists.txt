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
find_package(KF5DBusAddons ${KF5_MIN_VERSION} REQUIRED)
find_package(KF5Declarative ${KF5_MIN_VERSION} REQUIRED)
find_package(KF5Package REQUIRED)

configure_file(config-ktexteditor.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ktexteditor.h )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-plasma.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-plasma.h)

add_definitions(-DPLASMA_DEPRECATED=)

set(scripting_SRC
    scripting/appinterface.cpp
    scripting/applet.cpp
    scripting/containment.cpp
    scripting/configgroup.cpp
    scripting/i18n.cpp
    scripting/panel.cpp
    scripting/rect.cpp
    scripting/scriptengine.cpp
    scripting/scriptengine_v1.cpp
    scripting/widget.cpp
)

set(plasmashell_dbusXML dbus/org.kde.PlasmaShell.xml)
qt5_add_dbus_adaptor(scripting_SRC ${plasmashell_dbusXML} shellcorona.h ShellCorona plasmashelladaptor)

ecm_qt_declare_logging_category(plasmashell HEADER debug.h
                                               IDENTIFIER PLASMASHELL
                                               CATEGORY_NAME kde.plasmashell
                                               DEFAULT_SEVERITY Info)

set (plasma_shell_SRCS
    alternativeshelper.cpp
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
    standaloneappcorona
    osd.cpp
    waylanddialogfilter.cpp
    coronatesthelper.cpp
    debug.cpp
    screenpool.cpp
    ${scripting_SRC}
)

set(krunner_xml ${plasma-workspace_SOURCE_DIR}/krunner/dbus/org.kde.krunner.App.xml)
qt5_add_dbus_interface(plasma_shell_SRCS ${krunner_xml} krunner_interface)

add_executable(plasmashell
    ${plasma_shell_SRCS}
)

target_link_libraries(plasmashell
 Qt5::Quick
 Qt5::DBus
 KF5::KIOCore
 KF5::WindowSystem
 KF5::Crash
 KF5::Plasma
 KF5::PlasmaQuick
 Qt5::Script
 KF5::Solid
 KF5::Declarative
 KF5::I18n
 KF5::IconThemes
 KF5::Activities
 KF5::GlobalAccel
 KF5::CoreAddons
 KF5::DBusAddons
 KF5::Declarative
 KF5::QuickAddons
 KF5::XmlGui
 KF5::Package
 KF5::WaylandClient
)
target_include_directories(plasmashell PRIVATE "${CMAKE_BINARY_DIR}")
target_compile_definitions(plasmashell PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

if(HAVE_X11)
  target_link_libraries(plasmashell ${X11_LIBRARIES} ${XCB_LIBRARIES} )
  target_link_libraries(plasmashell Qt5::X11Extras)
endif()

install(TARGETS plasmashell ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES plasmashell.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
install( FILES dbus/org.kde.PlasmaShell.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )

install(FILES
   scripting/plasma-layout-template.desktop
   DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})

add_subdirectory(packageplugins)
add_subdirectory(autotests)
