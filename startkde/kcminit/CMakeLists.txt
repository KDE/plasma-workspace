########### next target ###############

set(kcminit_SRCS main.cpp)

add_executable(kcminit ${kcminit_SRCS})

target_link_libraries(kcminit Qt::Core Qt::Gui Qt::DBus KF5::CoreAddons KF5::Service KF5::I18n PW::KWorkspace)

install(TARGETS kcminit         ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

########### next target ###############

# TODO might be simpler to make <whatever>_startup to be a symlink to <whatever>

set(kcminit_startup_SRCS main.cpp)

add_executable(kcminit_startup ${kcminit_startup_SRCS})

ecm_install_configured_files(INPUT plasma-kcminit-phase1.service.in  plasma-kcminit.service.in
                                                   DESTINATION ${SYSTEMD_USER_UNIT_INSTALL_DIR})

target_link_libraries(kcminit_startup Qt::Core Qt::Gui Qt::DBus KF5::CoreAddons KF5::Service KF5::I18n PW::KWorkspace)

install(TARGETS kcminit_startup         ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
