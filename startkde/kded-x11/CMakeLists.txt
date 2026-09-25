add_executable(plasma-kded6-x11 kded.cpp)

ecm_qt_declare_logging_category(
    plasma-kded6-x11
    HEADER kded_debug.h
    IDENTIFIER KDED
    CATEGORY_NAME org.kde.plasma.kded.x11
    DESCRIPTION "KDED X11"
    EXPORT PLASMAWORKSPACE
)

target_link_libraries(plasma-kded6-x11
    Qt6::Widgets # QApplication
    KF6::CoreAddons # Needed for KPluginFactory
    KF6::DBusAddons # Needed for kdedmodule.h
    KF6::ConfigCore
)

install(TARGETS plasma-kded6-x11 ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

ecm_qt_install_logging_categories(
    EXPORT PLASMAWORKSPACE
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
