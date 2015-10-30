set(sessionsprivateplugin_SRCS
    sessionsmodel.cpp
    sessionsprivateplugin.cpp
)

qt5_add_dbus_interface(sessionsprivateplugin_SRCS ${plasma-workspace_SOURCE_DIR}/ksmserver/screenlocker/dbus/org.freedesktop.ScreenSaver.xml screensaver_interface)

kconfig_add_kcfg_files(sessionsprivateplugin_SRCS ${plasma-workspace_SOURCE_DIR}/ksmserver/screenlocker/kcfg/kscreensaversettings.kcfgc)

add_library(sessionsprivateplugin SHARED ${sessionsprivateplugin_SRCS})
target_link_libraries(sessionsprivateplugin
    Qt5::Core
    Qt5::DBus
    Qt5::Quick
    Qt5::Qml
    Qt5::Gui
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::ConfigGui
    PW::KWorkspace
)

install(TARGETS sessionsprivateplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/sessions)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/sessions)

