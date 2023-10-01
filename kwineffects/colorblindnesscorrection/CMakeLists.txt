# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"kwin_effects_colorblindnesscorrection\")

kcoreaddons_add_plugin(kwin_effect_colorblindnesscorrection INSTALL_NAMESPACE "kwin/effects/plugins/")
target_sources(kwin_effect_colorblindnesscorrection PRIVATE
    colorblindnesscorrection.cpp
    colorblindnesscorrection.qrc
    main.cpp
)
kconfig_add_kcfg_files(kwin_effect_colorblindnesscorrection colorblindnesscorrection_settings.kcfgc GENERATE_MOC)

target_link_libraries(kwin_effect_colorblindnesscorrection PRIVATE
    KWinEffects::kwineffects
    KWinEffects::kwinglutils
    KF6::ConfigCore
    KF6::ConfigGui
)

# Config
kcmutils_add_qml_kcm(kwin_colorblindnesscorrection_config SOURCES colorblindnesscorrection_config.cpp INSTALL_NAMESPACE "kwin/effects/configs")
kcmutils_generate_module_data(kwin_colorblindnesscorrection_config
    MODULE_DATA_HEADER colorblindnesscorrection_settingsdata.h
    MODULE_DATA_CLASS_NAME ColorBlindnessCorrectionSettingsData
    SETTINGS_HEADERS colorblindnesscorrection_settings.h
    SETTINGS_CLASSES ColorBlindnessCorrectionSettings
)
kconfig_add_kcfg_files(kwin_colorblindnesscorrection_config colorblindnesscorrection_settings.kcfgc GENERATE_MOC)

target_link_libraries(kwin_colorblindnesscorrection_config
    Qt::DBus
    KF6::ConfigCore
    KF6::I18n
    KF6::KCMUtils
    KF6::KCMUtilsQuick
)
