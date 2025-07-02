add_definitions(-DTRANSLATION_DOMAIN=\"kcm_nighttime\")

kcmutils_add_qml_kcm(kcm_nighttime)

target_sources(kcm_nighttime PRIVATE
    darklightschedulepreview.cpp
    darklightschedulevalidator.cpp
    dashedbackground.cpp
    kcm.cpp
    sunpathchart.cpp
)

kcmutils_generate_module_data(kcm_nighttime
    MODULE_DATA_HEADER nighttimedata.h
    MODULE_DATA_CLASS_NAME NightTimeData
    SETTINGS_HEADERS nighttimesettings.h
    SETTINGS_CLASSES NightTimeSettings
)

kconfig_target_kcfg_file(kcm_nighttime
    FILE nighttimesettings.kcfg
    CLASS_NAME NightTimeSettings
    GENERATE_MOC
    GENERATE_PROPERTIES
    MUTATORS
    NOTIFIERS
    PARENT_IN_CONSTRUCTOR
)

target_link_libraries(kcm_nighttime PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Positioning
    Qt6::Quick
    KF6::Holidays
    KF6::I18n
    KF6::KCMUtils
    KF6::KCMUtilsQuick
    KNightTime
)
