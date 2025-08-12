add_definitions(-DTRANSLATION_DOMAIN=\"kcm_lookandfeel\")

add_subdirectory(kded)
add_subdirectory(tool)

kcmutils_add_qml_kcm(kcm_lookandfeel)
target_sources(kcm_lookandfeel PRIVATE
    kcm.cpp
)
target_link_libraries(kcm_lookandfeel PRIVATE
    KF6::ConfigGui
    KF6::KCMUtils
    KF6::KCMUtilsQuick
    KF6::NewStuffCore
    PW::KLookAndFeel
)

kcmutils_generate_module_data(kcm_lookandfeel
    MODULE_DATA_HEADER lookandfeeldata.h
    MODULE_DATA_CLASS_NAME LookAndFeelData
    SETTINGS_HEADERS lookandfeelsettings.h
    SETTINGS_CLASSES LookAndFeelSettings
)

kconfig_add_kcfg_files(kcm_lookandfeel lookandfeelsettings.kcfgc GENERATE_MOC)

install(FILES lookandfeelsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES lookandfeel.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
