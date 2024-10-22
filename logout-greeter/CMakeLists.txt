add_definitions(-DTRANSLATION_DOMAIN=\"ksmserver-logout-greeter\")

set(LOGOUT_GREETER_SRCS main.cpp greeter.cpp shutdowndlg.cpp)
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
    X11::X11
    LayerShellQt::Interface
)

if(PACKAGEKIT_OFFLINE_UPDATES)
    target_link_libraries(ksmserver-logout-greeter PK::packagekitqt6)
    target_compile_definitions(ksmserver-logout-greeter PRIVATE PACKAGEKIT_OFFLINE_UPDATES)
endif()

target_link_libraries(ksmserver-logout-greeter Qt::GuiPrivate)
install(TARGETS ksmserver-logout-greeter DESTINATION ${KDE_INSTALL_LIBEXECDIR})
kdbusaddons_generate_dbus_service_file(ksmserver-logout-greeter org.kde.LogoutPrompt ${KDE_INSTALL_FULL_LIBEXECDIR})

ecm_add_qml_module(ksmserver-logout-greater-plugin-private URI "org.kde.plasma.private.logout-greeter" GENERATE_PLUGIN_SOURCE VERSION 1.0)
ecm_target_qml_sources(ksmserver-logout-greater-plugin-private SOURCES Error.qml)
ecm_finalize_qml_module(ksmserver-logout-greater-plugin-private DESTINATION ${KDE_INSTALL_QMLDIR})

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
