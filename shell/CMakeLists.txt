set(USE_SCRIPTING 1)

configure_file(config-ktexteditor.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ktexteditor.h )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-plasma.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-plasma.h)

add_definitions(-DPLASMA_DEPRECATED=)

set(scripting_SRC
    scripting/appinterface.cpp
    scripting/applet.cpp
    scripting/containment.cpp
    scripting/configgroup.cpp
    scripting/panel.cpp
    scripting/scriptengine.cpp
    scripting/scriptengine_v1.cpp
    scripting/widget.cpp
)

set(plasmashell_dbusXML dbus/org.kde.PlasmaShell.xml)
qt_add_dbus_adaptor(scripting_SRC ${plasmashell_dbusXML} shellcorona.h ShellCorona plasmashelladaptor)

ecm_qt_declare_logging_category(plasmashell HEADER debug.h
                                               IDENTIFIER PLASMASHELL
                                               CATEGORY_NAME kde.plasmashell
                                               DEFAULT_SEVERITY Info
                                               DESCRIPTION "plasmashell"
                                               EXPORT PLASMAWORKSPACE)

ecm_qt_declare_logging_category(plasmashell HEADER screenpool-debug.h
                                               IDENTIFIER SCREENPOOL
                                               CATEGORY_NAME kde.plasmashell.screenpool
                                               DEFAULT_SEVERITY Info
                                               DESCRIPTION "plasmashell screenpool"
                                               EXPORT PLASMAWORKSPACE
                                           )
set (plasma_shell_SRCS
    alternativeshelper.cpp
    autohidescreenedge.cpp
    main.cpp
    containmentconfigview.cpp
    currentcontainmentactionsmodel.cpp
    desktopview.cpp
    panelview.cpp
    panelconfigview.cpp
    panelshadows.cpp
    outputorderwatcher.cpp
    shellcorona.cpp
    osd.cpp
    coronatesthelper.cpp
    strutmanager.cpp
    debug.cpp
    screenpool-debug.cpp
    screenpool.cpp
    softwarerendernotifier.cpp
    shellcontainmentconfig.cpp
    ${scripting_SRC}
)

if (TARGET KF6::UserFeedbackCore)
    set(plasma_shell_SRCS
        ${plasma_shell_SRCS}
        userfeedback.cpp
    )
endif()

kconfig_add_kcfg_files(plasma_shell_SRCS ../kcms/colors/colorssettings.kcfgc GENERATE_MOC)

add_executable(plasmashell)
qt6_generate_wayland_protocol_client_sources(plasmashell FILES
    ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-output-order-v1.xml
    ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-screen-edge-v1.xml
)

set(krunner_xml ${plasma-workspace_SOURCE_DIR}/krunner/dbus/org.kde.krunner.App.xml)
qt_add_dbus_interface(plasma_shell_SRCS ${krunner_xml} krunner_interface)


target_sources(plasmashell PRIVATE ${plasma_shell_SRCS})

target_link_libraries(plasmashell
 Qt::Quick
 Qt::DBus
 Qt::WaylandClient
 KF6::KIOCore
 KF6::WindowSystem
 KF6::ConfigCore # Activate KRunner on the desktop
 KF6::Crash
 KF6::Plasma
 KF6::PlasmaQuick
 KF6::Solid
 KF6::Svg
 KF6::I18n
 KF6::Activities
 KF6::GlobalAccel
 KF6::CoreAddons
 KF6::DBusAddons
 KF6::XmlGui
 KF6::WidgetsAddons
 KF6::ConfigGui
 KF6::Package
 Plasma::KWaylandClient
 KF6::Notifications
 KF6::GuiAddons
 KF6::Service
 KF6::StatusNotifierItem
 PW::KWorkspace
 Wayland::Client
 LayerShellQt::Interface
)
target_link_libraries(plasmashell KF6::ConfigQml)
if (TARGET KF6::UserFeedbackCore)
    target_link_libraries(plasmashell KF6::UserFeedbackCore)
    target_compile_definitions(plasmashell PRIVATE -DWITH_KUSERFEEDBACKCORE)
endif()

target_include_directories(plasmashell PRIVATE "${CMAKE_BINARY_DIR}")
target_compile_definitions(plasmashell PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

if(HAVE_X11)
  target_link_libraries(plasmashell XCB::XCB XCB::RANDR)
  target_link_libraries(plasmashell Qt::GuiPrivate)
endif()

configure_file(org.kde.plasmashell.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.desktop @ONLY)

install(TARGETS plasmashell ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
install( FILES dbus/org.kde.PlasmaShell.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )

ecm_install_configured_files(INPUT plasma-plasmashell.service.in @ONLY DESTINATION  ${KDE_INSTALL_SYSTEMDUSERUNITDIR})

add_subdirectory(packageplugins)
if(BUILD_TESTING)
   add_subdirectory(autotests)
   add_subdirectory(tests)
endif()

add_subdirectory(kconf_update)
