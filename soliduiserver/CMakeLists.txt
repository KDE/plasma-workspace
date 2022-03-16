########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"soliduiserver5\")

kcoreaddons_add_plugin(soliduiserver SOURCES soliduiserver.cpp soliduiserver.h INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kded")
target_link_libraries(soliduiserver
    KF5::Solid
    KF5::DBusAddons
    KF5::Wallet
    KF5::KIOCore
    KF5::WindowSystem
    KF5::I18n
    KF5::WidgetsAddons
)


ecm_qt_declare_logging_category(soliduiserver
    HEADER soliduiserver_debug.h
    IDENTIFIER SOLIDUISERVER_DEBUG
    CATEGORY_NAME org.kde.plasma.soliduiserver
)
