ecm_find_qmlmodule(org.kde.plasma.core 2.0)

# KI18N Translation Domain for this library.
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_translations\")

########### next target ###############

set(kcm_translations_PART_SRCS translations.cpp translationsmodel.cpp translationssettings.cpp language.cpp)

kcmutils_generate_module_data(
    kcm_translations_PART_SRCS
    MODULE_DATA_HEADER translationsdata.h
    MODULE_DATA_CLASS_NAME TranslationsData
    SETTINGS_HEADERS translationssettings.h
    SETTINGS_CLASSES TranslationsSettings
)

kconfig_add_kcfg_files(kcm_translations_PART_SRCS translationssettingsbase.kcfgc GENERATE_MOC)

ecm_qt_declare_logging_category(
    kcm_translations_PART_SRCS
        HEADER debug.h
        IDENTIFIER KCM_TRANSLATIONS
        CATEGORY_NAME org.kde.kcm_translations
        DESCRIPTION "Translations KCM"
        EXPORT kcm_translations
)

ecm_qt_install_logging_categories(
    EXPORT kcm_translations
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)

kcoreaddons_add_plugin(kcm_translations SOURCES ${kcm_translations_PART_SRCS} INSTALL_NAMESPACE "kcms")

target_link_libraries(kcm_translations
    KF5::I18n
    KF5::KCMUtils
    KF5::QuickAddons
)

if(HAVE_PACKAGEKIT)
    target_link_libraries(kcm_translations PK::packagekitqt5)
endif()

kcoreaddons_desktop_to_json(kcm_translations "kcm_translations.desktop")

########### install files ###############
install(FILES kcm_translations.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR})
kpackage_install_package(package kcm_translations kcms)

