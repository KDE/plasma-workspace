add_definitions(-DTRANSLATION_DOMAIN=\"kcm_desktoptheme\")

kcmutils_add_qml_kcm(kcm_desktoptheme SOURCES ${kcm_desktoptheme_SRCS})

target_sources(kcm_desktoptheme PRIVATE
    kcm.cpp
    themesmodel.cpp
    filterproxymodel.cpp
    kcm.h
    themesmodel.h
    filterproxymodel.h
)
kcmutils_generate_module_data(kcm_desktoptheme
    MODULE_DATA_HEADER desktopthemedata.h
    MODULE_DATA_CLASS_NAME DesktopThemeData
    SETTINGS_HEADERS desktopthemesettings.h
    SETTINGS_CLASSES DesktopThemeSettings
)
kconfig_add_kcfg_files(kcm_desktoptheme desktopthemesettings.kcfgc GENERATE_MOC)

target_link_libraries(kcm_desktoptheme PRIVATE
  Qt::Quick
  KF6::CoreAddons
  KF6::KCMUtils
  KF6::KIOCore
  KF6::KIOWidgets
  KF6::I18n
  KF6::Svg
  KF6::KCMUtilsQuick
)

add_executable(plasma-apply-desktoptheme ${plasma-apply-desktoptheme_SRCS})
target_sources(plasma-apply-desktoptheme PRIVATE
    plasma-apply-desktoptheme.cpp
    themesmodel.cpp
)
target_link_libraries(plasma-apply-desktoptheme
    KF6::CoreAddons
    KF6::KCMUtils
    KF6::I18n
    Plasma::Plasma
)

#this desktop file is installed only for retrocompatibility with sycoca
install(FILES plasma-themes.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
install(TARGETS plasma-apply-desktoptheme DESTINATION ${KDE_INSTALL_BINDIR})
