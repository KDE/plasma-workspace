set(LOGOUT_GREETER_SRCS main.cpp greeter.cpp shutdowndlg.cpp shutdowndlg.h)
ecm_qt_declare_logging_category(LOGOUT_GREETER_SRCS HEADER debug.h
                                IDENTIFIER LOGOUT_GREETER
                                CATEGORY_NAME kde.logout_greeter
                                DEFAULT_SEVERITY Info
                                DESCRIPTION "logout greeter debug"
                                EXPORT PLASMAWORKSPACE
                            )


qt_add_dbus_adaptor( LOGOUT_GREETER_SRCS ../ksmserver/org.kde.LogoutPrompt.xml greeter.h Greeter)
add_executable(ksmserver-logout-greeter ${LOGOUT_GREETER_SRCS})
target_link_libraries(ksmserver-logout-greeter
    PW::KWorkspace
    Qt::Widgets
    Qt::Quick
    Qt::DBus
    KF6::I18n
    Plasma::PlasmaQuick
    KF6::Package
    KF6::WindowSystem
    KF6::ConfigCore
    Qt::WaylandClient
    Qt::WaylandClientPrivate
    X11::X11
    LayerShellQt::Interface
    Wayland::Client
)

if(PACKAGEKIT_OFFLINE_UPDATES)
    target_link_libraries(ksmserver-logout-greeter PK::packagekitqt6)
    target_compile_definitions(ksmserver-logout-greeter PRIVATE PACKAGEKIT_OFFLINE_UPDATES)
endif()

qt6_generate_wayland_protocol_client_sources(ksmserver-logout-greeter FILES
    ${PLASMA_WAYLAND_PROTOCOLS_DIR}/kde-lockscreen-overlay-v1.xml
)

install(TARGETS ksmserver-logout-greeter DESTINATION ${KDE_INSTALL_LIBEXECDIR})
configure_file(org.kde.ksmserver.greeter.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/org.kde.ksmserver.greeter.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.ksmserver.greeter.desktop DESTINATION ${KDE_INSTALL_APPDIR})
kdbusaddons_generate_dbus_service_file(ksmserver-logout-greeter org.kde.LogoutPrompt ${KDE_INSTALL_FULL_LIBEXECDIR})

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
