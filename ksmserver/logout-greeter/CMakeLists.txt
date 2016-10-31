set(KSMSERVER_LOGOUT_GREETER_SRCS main.cpp ../shutdowndlg.cpp)
add_executable(ksmserver-logout-greeter ${KSMSERVER_LOGOUT_GREETER_SRCS})
target_link_libraries(ksmserver-logout-greeter
    PW::KWorkspace
    Qt5::Widgets
    Qt5::Quick
    Qt5::X11Extras
    KF5::Declarative
    KF5::IconThemes
    KF5::I18n
    KF5::Package
    KF5::QuickAddons
    KF5::KDELibs4Support # Solid/PowerManagement
    KF5::WaylandClient
    ${X11_LIBRARIES}
)
install(TARGETS ksmserver-logout-greeter DESTINATION ${KDE_INSTALL_LIBEXECDIR})

add_subdirectory(tests)
