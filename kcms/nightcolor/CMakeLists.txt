# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_nightcolor\")


set(kcm_nightcolor_SRCS
    kcm.cpp
    enum.h
)

kcmutils_generate_module_data(
    kcm_nightcolor_SRCS
    MODULE_DATA_HEADER nightcolordata.h
    MODULE_DATA_CLASS_NAME NightColorData
    SETTINGS_HEADERS nightcolorsettings.h
    SETTINGS_CLASSES NightColorSettings
)

kconfig_add_kcfg_files(kcm_nightcolor_SRCS nightcolorsettings.kcfgc GENERATE_MOC)

kcoreaddons_add_plugin(kcm_nightcolor SOURCES ${kcm_nightcolor_SRCS} INSTALL_NAMESPACE "plasma/kcms/systemsettings")
kcmutils_generate_desktop_file(kcm_nightcolor)

target_link_libraries(kcm_nightcolor
    KF5::QuickAddons
    KF5::I18n
    KF5::KCMUtils
    PW::LibColorCorrect
    Qt::DBus
)

install(FILES worldmap.png DESTINATION ${KDE_INSTALL_DATADIR}/plasma/nightcolor)

kpackage_install_package(package kcm_nightcolor kcms)
