########### next target ###############

set(kcminit_KDEINIT_SRCS main.cpp)

set(klauncher_xml ${KINIT_DBUS_INTERFACES_DIR}/kf5_org.kde.KLauncher.xml)
qt5_add_dbus_interface(kcminit_KDEINIT_SRCS ${klauncher_xml} klauncher_iface)

kf5_add_kdeinit_executable( kcminit ${kcminit_KDEINIT_SRCS})

target_link_libraries(kdeinit_kcminit Qt::Core Qt::Gui Qt::DBus KF5::CoreAddons KF5::Service KF5::I18n PW::KWorkspace)

install(TARGETS kdeinit_kcminit ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
install(TARGETS kcminit         ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

########### next target ###############

# TODO might be simpler to make <whatever>_startup to be a symlink to <whatever>

set(kcminit_startup_KDEINIT_SRCS main.cpp)

qt5_add_dbus_interface(kcminit_startup_KDEINIT_SRCS ${klauncher_xml} klauncher_iface)
kf5_add_kdeinit_executable( kcminit_startup ${kcminit_startup_KDEINIT_SRCS})

ecm_install_configured_files(INPUT plasma-kcminit-phase1.service.in  plasma-kcminit.service.in
                                                   DESTINATION ${SYSTEMD_USER_UNIT_INSTALL_DIR})

target_link_libraries(kdeinit_kcminit_startup Qt::Core Qt::Gui Qt::DBus KF5::CoreAddons KF5::Service KF5::I18n PW::KWorkspace)

install(TARGETS kdeinit_kcminit_startup ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
install(TARGETS kcminit_startup         ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

