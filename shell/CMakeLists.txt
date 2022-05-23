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
                                               DEFAULT_SEVERITY Info)

ecm_qt_declare_logging_category(plasmashell HEADER screenpool-debug.h
                                               IDENTIFIER SCREENPOOL
                                               CATEGORY_NAME kde.plasmashell.screenpool
                                               DEFAULT_SEVERITY Info)
set (plasma_shell_SRCS
    alternativeshelper.cpp
    main.cpp
    containmentconfigview.cpp
    currentcontainmentactionsmodel.cpp
    desktopview.cpp
    panelview.cpp
    panelconfigview.cpp
    panelshadows.cpp
    primaryoutputwatcher.cpp
    shellcorona.cpp
    standaloneappcorona.cpp
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

if (TARGET KUserFeedbackCore)
    set(plasma_shell_SRCS
        ${plasma_shell_SRCS}
        userfeedback.cpp
    )
endif()

kconfig_add_kcfg_files(plasma_shell_SRCS ../kcms/colors/colorssettings.kcfgc GENERATE_MOC)

add_executable(plasmashell)
if (QT_MAJOR_VERSION EQUAL "5")
    ecm_add_qtwayland_client_protocol(plasma_shell_SRCS
        PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-primary-output-v1.xml
        BASENAME kde-primary-output-v1
    )
else()
    qt6_generate_wayland_protocol_client_sources(plasmashell FILES
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-primary-output-v1.xml
    )
endif()

set(krunner_xml ${plasma-workspace_SOURCE_DIR}/krunner/dbus/org.kde.krunner.App.xml)
qt_add_dbus_interface(plasma_shell_SRCS ${krunner_xml} krunner_interface)


target_sources(plasmashell PRIVATE ${plasma_shell_SRCS})

target_include_directories(plasmashell PRIVATE ${Libdrm_INCLUDE_DIR})

target_link_libraries(plasmashell
 Qt::Quick
 Qt::DBus
 KF5::KIOCore
 KF5::WindowSystem
 KF5::ConfigCore # Activate KRunner on the desktop
 KF5::Crash
 KF5::Plasma
 KF5::PlasmaQuick
 KF5::Solid
 KF5::Declarative
 KF5::I18n
 KF5::Activities
 KF5::GlobalAccel
 KF5::CoreAddons
 KF5::DBusAddons
 KF5::QuickAddons
 KF5::XmlGui
 KF5::Package
 KF5::WaylandClient
 KF5::Notifications
 KF5::GuiAddons
 PW::KWorkspace
 Wayland::Client
 Libdrm::Libdrm
)
if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(plasmashell KF5::ConfigQml)
endif()
if (TARGET KUserFeedbackCore)
    target_link_libraries(plasmashell KUserFeedbackCore)
    target_compile_definitions(plasmashell PRIVATE -DWITH_KUSERFEEDBACKCORE)
endif()

target_include_directories(plasmashell PRIVATE "${CMAKE_BINARY_DIR}")
target_compile_definitions(plasmashell PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

if(HAVE_X11)
  target_link_libraries(plasmashell XCB::XCB XCB::RANDR)
  if (QT_MAJOR_VERSION EQUAL "5")
    target_link_libraries(plasmashell Qt::X11Extras)
  else()
    target_link_libraries(plasmashell Qt::GuiPrivate)
  endif()
endif()

configure_file(org.kde.plasmashell.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.desktop @ONLY)

install(TARGETS plasmashell ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
install( FILES dbus/org.kde.PlasmaShell.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )

ecm_install_configured_files(INPUT plasma-plasmashell.service.in @ONLY DESTINATION  ${KDE_INSTALL_SYSTEMDUSERUNITDIR})

install(FILES
   scripting/plasma-layouttemplate.desktop
   DESTINATION ${KDE_INSTALL_KSERVICETYPESDIR})

add_subdirectory(packageplugins)
if(BUILD_TESTING)
   add_subdirectory(autotests)
   add_subdirectory(tests)
endif()
