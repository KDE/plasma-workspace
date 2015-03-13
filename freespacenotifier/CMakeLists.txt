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

install(TARGETS kded_freespacenotifier  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES freespacenotifier.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR}/kded )
install( FILES freespacenotifier.notifyrc  DESTINATION  ${KDE_INSTALL_KNOTIFY5RCDIR} )
install( FILES freespacenotifier.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
