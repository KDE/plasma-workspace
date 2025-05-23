add_definitions(-DTRANSLATION_DOMAIN=\"freespacenotifier\")

set(kded_freespacenotifier_SRCS freespacenotifier.cpp module.cpp freespacenotifier.h module.h)

ecm_qt_declare_logging_category(kded_freespacenotifier_SRCS
    HEADER freespacenotifier_logging.h
    IDENTIFIER FSN
    CATEGORY_NAME org.kde.freespacenotifier
    DESCRIPTION "Free space notifier kded module"
)

ki18n_wrap_ui(kded_freespacenotifier_SRCS freespacenotifier_prefs_base.ui)

qt_add_dbus_interface(kded_freespacenotifier_SRCS ${KDED_DBUS_INTERFACE} kded_interface)

kconfig_add_kcfg_files(kded_freespacenotifier_SRCS settings.kcfgc)

kcoreaddons_add_plugin(freespacenotifier SOURCES ${kded_freespacenotifier_SRCS} INSTALL_NAMESPACE "kf6/kded")

kde_target_enable_exceptions(freespacenotifier PRIVATE)

target_link_libraries(freespacenotifier
    Qt6::Concurrent
    KF6::ConfigWidgets
    KF6::DBusAddons
    KF6::I18n
    KF6::KIOCore
    KF6::KIOGui
    KF6::Notifications
    KF6::JobWidgets
    KF6::Service
    KF6::Solid
)

########### install files ###############

install( FILES freespacenotifier.notifyrc  DESTINATION  ${KDE_INSTALL_KNOTIFYRCDIR} )
install( FILES freespacenotifier.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
