add_definitions(-DTRANSLATION_DOMAIN=\"kcm_icons\")

kcmutils_add_qml_kcm(kcm_icons)

target_sources(kcm_icons PRIVATE main.cpp iconsmodel.cpp iconsizecategorymodel.cpp iconssettings.cpp ../kcms-common.cpp)
kcmutils_generate_module_data(kcm_icons
    MODULE_DATA_HEADER iconsdata.h
    MODULE_DATA_CLASS_NAME IconsData
    SETTINGS_HEADERS iconssettings.h
    SETTINGS_CLASSES IconsSettings
)
kconfig_add_kcfg_files(kcm_icons iconssettingsbase.kcfgc GENERATE_MOC)

target_link_libraries(kcm_icons
    Qt::Widgets
    Qt::Svg
    Qt::Quick
    KF6::KCMUtils
    KF6::I18n
    KF6::IconThemes
    KF6::Archive
    KF6::KIOWidgets
    KF6::KCMUtilsQuick
)

file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config.h CONTENT "#define CMAKE_INSTALL_FULL_LIBEXECDIR \"${CMAKE_INSTALL_FULL_LIBEXECDIR}\"")

set(changeicons_SRCS changeicons.cpp iconssettings.cpp)

kconfig_add_kcfg_files(changeicons_SRCS iconssettingsbase.kcfgc GENERATE_MOC)

add_executable(plasma-changeicons ${changeicons_SRCS})
target_link_libraries(plasma-changeicons PRIVATE Qt::Core KF6::KIOWidgets KF6::KCMUtils KF6::IconThemes KF6::I18n)

ecm_qt_declare_logging_category(plasma-changeicons
    HEADER plasma_changeicons_debug.h
    IDENTIFIER PLASMA_CHANGEICONS_DEBUG
    CATEGORY_NAME org.kde.plasma.changeicons
)

install(FILES iconssettingsbase.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES icons_remove_effects.upd DESTINATION ${KDE_INSTALL_DATADIR}/kconf_update)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/icons.knsrc ${CMAKE_BINARY_DIR}/icons.knsrc)
install( FILES ${CMAKE_BINARY_DIR}/icons.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR} )

install(TARGETS plasma-changeicons DESTINATION ${KDE_INSTALL_LIBEXECDIR} )

