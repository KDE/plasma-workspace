add_definitions(-DTRANSLATION_DOMAIN=\"freespacenotifier\")

set(kded_freespacenotifier_SRCS freespacenotifier.cpp module.cpp)

ki18n_wrap_ui(kded_freespacenotifier_SRCS freespacenotifier_prefs_base.ui)

kconfig_add_kcfg_files(kded_freespacenotifier_SRCS settings.kcfgc)

add_library(freespacenotifier MODULE ${kded_freespacenotifier_SRCS})
kcoreaddons_desktop_to_json(freespacenotifier freespacenotifier.desktop)

target_link_libraries(freespacenotifier
    KF5::ConfigWidgets
    KF5::DBusAddons
    KF5::I18n
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::Notifications
)

install(TARGETS freespacenotifier  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded )


########### install files ###############

install( FILES freespacenotifier.notifyrc  DESTINATION  ${KDE_INSTALL_KNOTIFY5RCDIR} )
install( FILES freespacenotifier.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
