set(KSMSERVER_SWITCHUSER_GREETER_SRCS main.cpp ../switchuserdialog.cpp ../ksmserver_debug.cpp)
add_executable(ksmserver-switchuser-greeter ${KSMSERVER_SWITCHUSER_GREETER_SRCS})
target_link_libraries(ksmserver-switchuser-greeter
    PW::KWorkspace
    Qt5::Quick
    Qt5::X11Extras
    KF5::Declarative
    KF5::IconThemes
    KF5::I18n
    KF5::Package
    KF5::WaylandClient
    KF5::WindowSystem
    ${X11_LIBRARIES}
)
install(TARGETS ksmserver-switchuser-greeter DESTINATION ${KDE_INSTALL_LIBEXECDIR})
