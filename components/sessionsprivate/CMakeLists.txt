set(sessionsprivateplugin_SRCS
    sessionsmodel.cpp
    sessionsprivateplugin.cpp
)

qt5_add_dbus_interface(sessionsprivateplugin_SRCS ${SCREENSAVER_DBUS_INTERFACE} screensaver_interface)

kconfig_add_kcfg_files(sessionsprivateplugin_SRCS kscreensaversettings.kcfgc)

add_library(sessionsprivateplugin SHARED ${sessionsprivateplugin_SRCS})
target_link_libraries(sessionsprivateplugin
    Qt::Core
    Qt::DBus
    Qt::Quick
    Qt::Qml
    Qt::Gui
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::ConfigGui
    KF5::I18n
    PW::KWorkspace
)

install(TARGETS sessionsprivateplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/sessions)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/sessions)

