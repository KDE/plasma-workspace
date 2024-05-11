set(sessionsprivateplugin_SRCS
    sessionsmodel.cpp
    types.h
)

qt_add_dbus_interface(sessionsprivateplugin_SRCS ${SCREENSAVER_DBUS_INTERFACE} screensaver_interface)

kconfig_add_kcfg_files(sessionsprivateplugin_SRCS kscreensaversettings.kcfgc)

ecm_add_qml_module(sessionsprivateplugin
    URI "org.kde.plasma.private.sessions"
    VERSION 2.0
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES QtCore
)

target_sources(sessionsprivateplugin PRIVATE ${sessionsprivateplugin_SRCS})

target_link_libraries(sessionsprivateplugin PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Quick
    Qt::Qml
    Qt::Gui
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::I18n
    PW::KWorkspace
)

ecm_finalize_qml_module(sessionsprivateplugin)
