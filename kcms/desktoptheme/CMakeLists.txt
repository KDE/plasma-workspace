# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_desktoptheme\")

set(kcm_desktoptheme_SRCS
  kcm.cpp
  themesmodel.cpp
  filterproxymodel.cpp
  kcm.h
  themesmodel.h
  filterproxymodel.h
)

kcmutils_generate_module_data(
    kcm_desktoptheme_SRCS
    MODULE_DATA_HEADER desktopthemedata.h
    MODULE_DATA_CLASS_NAME DesktopThemeData
    SETTINGS_HEADERS desktopthemesettings.h
    SETTINGS_CLASSES DesktopThemeSettings
)

kconfig_add_kcfg_files(kcm_desktoptheme_SRCS desktopthemesettings.kcfgc GENERATE_MOC)

kcmutils_add_qml_kcm(kcm_desktoptheme SOURCES ${kcm_desktoptheme_SRCS})

target_link_libraries(kcm_desktoptheme
  Qt::Quick
  KF6::CoreAddons
  KF6::KCMUtils
  KF6::KIOCore
  KF6::KIOWidgets
  KF6::I18n
  KF6::Plasma
  KF6::KCMUtilsQuick
)

set(plasma-apply-desktoptheme_SRCS
    plasma-apply-desktoptheme.cpp
    themesmodel.cpp
)

add_executable(plasma-apply-desktoptheme ${plasma-apply-desktoptheme_SRCS})

target_link_libraries(plasma-apply-desktoptheme
  KF6::CoreAddons
  KF6::KCMUtils
  KF6::I18n
  KF6::Plasma
)

#this desktop file is installed only for retrocompatibility with sycoca
install(FILES plasma-themes.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
install(TARGETS plasma-apply-desktoptheme DESTINATION ${KDE_INSTALL_BINDIR})
