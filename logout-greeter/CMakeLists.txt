set(LOGOUT_GREETER_SRCS main.cpp greeter.cpp shutdowndlg.cpp)
ecm_qt_declare_logging_category(LOGOUT_GREETER_SRCS HEADER debug.h
                                IDENTIFIER LOGOUT_GREETER
                                CATEGORY_NAME kde.logout_greeter
                                DEFAULT_SEVERITY Info)


qt5_add_dbus_adaptor( LOGOUT_GREETER_SRCS ../ksmserver/org.kde.LogoutPrompt.xml greeter.h Greeter)
qt5_add_dbus_interface( LOGOUT_GREETER_SRCS ../ksmserver/org.kde.KSMServerInterface.xml ksmserveriface)

add_executable(ksmserver-logout-greeter ${LOGOUT_GREETER_SRCS})
target_link_libraries(ksmserver-logout-greeter
    PW::KWorkspace
    Qt5::Widgets
    Qt5::Quick
    Qt5::X11Extras
    KF5::Declarative
    KF5::I18n
    KF5::Package
    KF5::QuickAddons
    KF5::KDELibs4Support # Solid/PowerManagement
    KF5::WaylandClient
    ${X11_LIBRARIES}
)
install(TARGETS ksmserver-logout-greeter DESTINATION ${KDE_INSTALL_LIBEXECDIR})
kdbusaddons_generate_dbus_service_file(ksmserver-logout-greeter org.kde.LogoutPrompt ${KDE_INSTALL_FULL_LIBEXECDIR})

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
