# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Arjen Hiemstra <ahiemstra@heimr.nl>

kcmutils_add_qml_kcm(kcm_union_style)

target_sources(kcm_union_style PRIVATE
    KcmUnionStyle.cpp
    KcmUnionStyle.h
)

kconfig_target_kcfg_file(kcm_union_style
    FILE unionsettings.kcfg
    CLASS_NAME UnionSettings
    MUTATORS
    GENERATE_MOC
    GENERATE_PROPERTIES
    QML_REGISTRATION
    PARENT_IN_CONSTRUCTOR
    NOTIFIERS
)

kcmutils_generate_module_data(kcm_union_style
    MODULE_DATA_HEADER unionstyledata.h
    MODULE_DATA_CLASS_NAME UnionStyleData
    SETTINGS_HEADERS unionsettings.h
    SETTINGS_CLASSES UnionSettings
)

target_link_libraries(kcm_union_style PRIVATE
    Union::Union
    Qt::Quick
    KF6::KCMUtils
    KF6::I18n
)

target_compile_definitions(kcm_union_style PRIVATE TRANSLATION_DOMAIN=\"kcm_union_style\")

ecm_add_qml_module(kcm_union_style_qml URI org.kde.private.kcms.union GENERATE_PLUGIN_SOURCE)

target_sources(kcm_union_style_qml PRIVATE
    StylesModel.cpp
    StylesModel.h
)

target_link_libraries(kcm_union_style_qml PRIVATE
    Union::Union
)

ecm_finalize_qml_module(kcm_union_style_qml)
