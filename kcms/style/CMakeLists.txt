# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_style\")

########### next target ###############

set(kcm_style_PART_SRCS
    ../kcms-common.cpp
    styleconfdialog.cpp
    kcmstyle.cpp
    stylesmodel.cpp
    gtkthemesmodel.cpp
    gtkpage.cpp
    previewitem.cpp
    styledata.cpp
    styleconfdialog.h
    kcmstyle.h
    stylesmodel.h
    gtkthemesmodel.h
    gtkpage.h
    previewitem.h
    styledata.h
)
qt_add_dbus_interface(kcm_style_PART_SRCS ${KDED_DBUS_INTERFACE} kded_interface)
qt_add_dbus_interface(kcm_style_PART_SRCS org.kde.GtkConfig.xml kdegtkconfig_interface)

kcmutils_generate_module_data(
    kcm_style_PART_SRCS
    MODULE_DATA_HEADER styledata.h
    MODULE_DATA_CLASS_NAME StyleData
    SETTINGS_HEADERS stylesettings.h
    SETTINGS_CLASSES StyleSettings
)

ki18n_wrap_ui(kcm_style_PART_SRCS stylepreview.ui)

kconfig_add_kcfg_files(kcm_style_PART_SRCS stylesettings.kcfgc GENERATE_MOC)

kcoreaddons_add_plugin(kcm_style SOURCES ${kcm_style_PART_SRCS} INSTALL_NAMESPACE "plasma/kcms/systemsettings")
kcmutils_generate_desktop_file(kcm_style)

target_compile_definitions(kcm_style PUBLIC CMAKE_INSTALL_FULL_LIBEXECDIR="${CMAKE_INSTALL_FULL_LIBEXECDIR}")

target_link_libraries(kcm_style
    KF5::KCMUtils
    KF5::I18n
    KF5::KIOCore
    KF5::QuickAddons
    KF5::Archive
    KF5::NewStuff
    krdb
)

ecm_qt_declare_logging_category(kcm_style
    HEADER kcm_style_debug.h
    IDENTIFIER KCM_STYLE_DEBUG
    CATEGORY_NAME org.kde.plasma.kcm_style
)

install(FILES stylesettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES style_widgetstyle_default_breeze.upd style_widgetstyle_default_breeze.pl DESTINATION ${KDE_INSTALL_DATADIR}/kconf_update)
install(FILES gtk_themes.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

kpackage_install_package(package kcm_style kcms)

add_custom_command(TARGET kcm_style POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ../kcms/systemsettings/kcm_style.so kcm_style_init.so)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kcm_style_init.so DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcminit)
