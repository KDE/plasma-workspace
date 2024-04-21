add_subdirectory(plugin)

ecm_qt_install_logging_categories(
        EXPORT APPLETS::BATTERYMONITOR
        FILE applets/batterymonitor.categories
        DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

plasma_install_package(package org.kde.plasma.battery)

