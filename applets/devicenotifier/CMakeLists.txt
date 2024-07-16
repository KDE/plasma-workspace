add_subdirectory(plugin)

ecm_qt_install_logging_categories(
        EXPORT APPLETS::DEVICENOTIFIER
        FILE applets/devicenotifier.categories
        DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

plasma_install_package(package org.kde.plasma.devicenotifier)

install(FILES openWithFileManager.desktop DESTINATION ${KDE_INSTALL_DATADIR}/solid/actions )
