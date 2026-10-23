add_executable(plasma-terminalintent-wrapper plasma-terminalintent-wrapper.cpp)
kconfig_add_kcfg_files(plasma-terminalintent-wrapper ../terminal_settings.kcfgc GENERATE_MOC)
target_link_libraries(plasma-terminalintent-wrapper PRIVATE
    Qt::DBus
    KF6::ConfigGui
    KF6::Service
    KF6::KIOGui
    KF6::DBusAddons
)

ecm_generate_dbus_service_file(
        NAME org.kde.plasma-terminalintent-wrapper
        EXECUTABLE "${KDE_INSTALL_FULL_LIBEXECDIR}/plasma-terminalintent-wrapper"
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

install(TARGETS plasma-terminalintent-wrapper DESTINATION ${KDE_INSTALL_LIBEXECDIR})
install(FILES org.kde.plasma-terminalintent-wrapper.desktop DESTINATION ${KDE_INSTALL_APPDIR})
