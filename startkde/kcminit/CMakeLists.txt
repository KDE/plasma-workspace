########### next target ###############

set(kcminit_KDEINIT_SRCS main.cpp)

set(klauncher_xml ${KINIT_DBUS_INTERFACES_DIR}/kf5_org.kde.KLauncher.xml)
qt5_add_dbus_interface(kcminit_KDEINIT_SRCS ${klauncher_xml} klauncher_iface)

kf5_add_kdeinit_executable( kcminit ${kcminit_KDEINIT_SRCS})

target_link_libraries(kdeinit_kcminit Qt5::Core KF5::KDE4Support)

install(TARGETS kdeinit_kcminit ${INSTALL_TARGETS_DEFAULT_ARGS} )
install(TARGETS kcminit         ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### next target ###############

# TODO might be simpler to make <whatever>_startup to be a symlink to <whatever>

set(kcminit_startup_KDEINIT_SRCS main.cpp)


qt5_add_dbus_interface(kcminit_startup_KDEINIT_SRCS ${klauncher_xml} klauncher_iface)
kf5_add_kdeinit_executable( kcminit_startup ${kcminit_startup_KDEINIT_SRCS})

target_link_libraries(kdeinit_kcminit_startup Qt5::Core KF5::KDE4Support)

install(TARGETS kdeinit_kcminit_startup ${INSTALL_TARGETS_DEFAULT_ARGS} )
install(TARGETS kcminit_startup         ${INSTALL_TARGETS_DEFAULT_ARGS} )

