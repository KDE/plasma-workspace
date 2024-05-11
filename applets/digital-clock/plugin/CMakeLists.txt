add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.digitalclock\")

find_package(IsoCodes)
set_package_properties(IsoCodes PROPERTIES
                       PURPOSE "Translation of country names in digital clock applet"
                       TYPE RUNTIME
                      )

ecm_add_qml_module(digitalclockplugin
    URI "org.kde.plasma.private.digitalclock"
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES QtCore
)

target_sources(digitalclockplugin PRIVATE
    timezonemodel.cpp
    timezonesi18n.cpp
    clipboardmenu.cpp
    applicationintegration.cpp
    timezonemodel.h
    timezonesi18n.h
    clipboardmenu.h
    applicationintegration.h
)

target_link_libraries(digitalclockplugin
    PRIVATE
        Qt::Core
        Qt::Qml
        Qt::Widgets # for QAction...
        KF6::CoreAddons
        KF6::I18n
        KF6::I18nLocaleData
        KF6::KIOGui
        KF6::Service
        ICU::i18n
        ICU::uc
)

ecm_finalize_qml_module(digitalclockplugin)
