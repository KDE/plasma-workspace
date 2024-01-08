########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"soliduiserver\")

kcoreaddons_add_plugin(soliduiserver SOURCES soliduiserver.cpp soliduiserver.h INSTALL_NAMESPACE "kf6/kded")
target_link_libraries(soliduiserver
    KF6::Solid
    KF6::DBusAddons
    KF6::Wallet
    KF6::KIOCore
    KF6::WindowSystem
    KF6::I18n
    KF6::WidgetsAddons
)


ecm_qt_declare_logging_category(soliduiserver
    HEADER soliduiserver_debug.h
    IDENTIFIER SOLIDUISERVER_DEBUG
    CATEGORY_NAME org.kde.plasma.soliduiserver
    DESCRIPTION "soliduiserver"
    EXPORT PLASMAWORKSPACE
)
