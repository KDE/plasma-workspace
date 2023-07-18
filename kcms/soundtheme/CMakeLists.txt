# SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"kcm_soundtheme\")

kcmutils_add_qml_kcm(kcm_soundtheme)
target_sources(kcm_soundtheme PRIVATE
    kcm_soundtheme.cpp
)

kcmutils_generate_module_data(kcm_soundtheme
    MODULE_DATA_HEADER soundthemedata.h
    MODULE_DATA_CLASS_NAME SoundThemeData
    SETTINGS_HEADERS soundthemesettings.h
    SETTINGS_CLASSES SoundThemeSettings
)

kconfig_add_kcfg_files(kcm_soundtheme soundthemesettings.kcfgc GENERATE_MOC)

ecm_qt_declare_logging_category(kcm_soundtheme
    HEADER kcm_soundtheme_debug.h
    IDENTIFIER KCM_SOUNDTHEME
    CATEGORY_NAME kcm_soundtheme
    EXPORT KCM_SOUNDTHEME
    DESCRIPTION "System Settings - Sound Theme"
)

target_link_libraries(kcm_soundtheme PRIVATE
    KF6::KCMUtils
    KF6::I18n
    KF6::KCMUtilsQuick
    PW::LibNotificationManager
    Canberra::Canberra
)

