kcoreaddons_add_plugin(autodarklightlnf INSTALL_NAMESPACE "kf6/kded")

target_sources(autodarklightlnf PRIVATE
    module.cpp
)

kconfig_target_kcfg_file(autodarklightlnf
    FILE settings.kcfg
    CLASS_NAME Settings
    MUTATORS
)

kconfig_target_kcfg_file(autodarklightlnf
    FILE state.kcfg
    CLASS_NAME State
    MUTATORS
)

target_link_libraries(autodarklightlnf PRIVATE
    Qt6::Core
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::DBusAddons
    KNightTime
)
