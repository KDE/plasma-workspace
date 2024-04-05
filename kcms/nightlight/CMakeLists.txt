add_definitions(-DTRANSLATION_DOMAIN=\"kcm_nightlight\")

kcmutils_add_qml_kcm(kcm_nightlight SOURCES kcm.cpp enum.h)

kcmutils_generate_module_data(kcm_nightlight
    MODULE_DATA_HEADER nightlightdata.h
    MODULE_DATA_CLASS_NAME NightLightData
    SETTINGS_HEADERS nightlightsettings.h
    SETTINGS_CLASSES NightLightSettings
)
kconfig_add_kcfg_files(kcm_nightlight nightlightsettings.kcfgc GENERATE_MOC)
target_link_libraries(kcm_nightlight PRIVATE
    KF6::KCMUtilsQuick
    KF6::I18n
    KF6::KCMUtils
    PW::LibColorCorrect
    Qt::DBus
)
