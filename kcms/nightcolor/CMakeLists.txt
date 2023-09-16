add_definitions(-DTRANSLATION_DOMAIN=\"kcm_nightcolor\")

kcmutils_add_qml_kcm(kcm_nightcolor SOURCES kcm.cpp enum.h)

kcmutils_generate_module_data(kcm_nightcolor
    MODULE_DATA_HEADER nightcolordata.h
    MODULE_DATA_CLASS_NAME NightColorData
    SETTINGS_HEADERS nightcolorsettings.h
    SETTINGS_CLASSES NightColorSettings
)
kconfig_add_kcfg_files(kcm_nightcolor nightcolorsettings.kcfgc GENERATE_MOC)
target_link_libraries(kcm_nightcolor PRIVATE
    KF6::KCMUtilsQuick
    KF6::I18n
    KF6::KCMUtils
    PW::LibColorCorrect
    Qt::DBus
)
