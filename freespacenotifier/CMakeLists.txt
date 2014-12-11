project (freespacenotifier)

add_definitions(-DTRANSLATION_DOMAIN=\"freespacenotifier\")

set(kded_freespacenotifier_SRCS freespacenotifier.cpp module.cpp)

ki18n_wrap_ui(kded_freespacenotifier_SRCS freespacenotifier_prefs_base.ui)

kconfig_add_kcfg_files(kded_freespacenotifier_SRCS settings.kcfgc)

add_library(kded_freespacenotifier MODULE ${kded_freespacenotifier_SRCS})
kcoreaddons_desktop_to_json(kded_freespacenotifier freespacenotifier.desktop)

target_link_libraries(kded_freespacenotifier
    KF5::ConfigWidgets
    KF5::DBusAddons
    KF5::I18n
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::Notifications
)

install(TARGETS kded_freespacenotifier  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES freespacenotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
install( FILES freespacenotifier.notifyrc  DESTINATION  ${KNOTIFYRC_INSTALL_DIR} )
install( FILES freespacenotifier.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
