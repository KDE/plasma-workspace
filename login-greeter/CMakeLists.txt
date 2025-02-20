# Due to copy pasted code from QtGreet
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY=0)
remove_definitions(-DQT_USE_QSTRINGBUILDER)

set(LOGIN_GREETER_SRCS main.cpp
    greetd/GreetdManager.cpp
    greetd/LoginManager.cpp
)

qt_add_resources(RESOURCE_FILES resources.qrc)

ecm_qt_declare_logging_category(LOGIN_GREETER_SRCS HEADER debug.h
                                IDENTIFIER LOGOUT_GREETER
                                CATEGORY_NAME kde.login_greeter
                                DEFAULT_SEVERITY Info
                                DESCRIPTION "login greeter debug"
                                EXPORT PLASMAWORKSPACE
                            )

add_executable(plasma-login-greeter ${LOGIN_GREETER_SRCS})
target_link_libraries(plasma-login-greeter
    PW::KWorkspace
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
target_sources(plasma-login-greeter PRIVATE ${RESOURCE_FILES})

install(TARGETS plasma-login-greeter DESTINATION ${KDE_INSTALL_LIBEXECDIR})

ecm_install_configured_files(INPUT plasma-login.service.in DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
