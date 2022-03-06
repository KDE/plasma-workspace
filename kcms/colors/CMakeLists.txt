# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_colors\")

set(kcm_colors_SRCS
    ../kcms-common.cpp
    colors.cpp
    colorsapplicator.cpp
    colorsmodel.cpp
    filterproxymodel.cpp
)

kcmutils_generate_module_data(
    kcm_colors_SRCS
    MODULE_DATA_HEADER colorsdata.h
    MODULE_DATA_CLASS_NAME ColorsData
    SETTINGS_HEADERS colorssettings.h
    SETTINGS_CLASSES ColorsSettings
)

# needed for krdb
kconfig_add_kcfg_files(kcm_colors_SRCS colorssettings.kcfgc GENERATE_MOC)

kcoreaddons_add_plugin(kcm_colors SOURCES ${kcm_colors_SRCS} INSTALL_NAMESPACE "plasma/kcms/systemsettings")
target_link_libraries(kcm_colors
    Qt::DBus
    KF5::KCMUtils
    KF5::CoreAddons
    KF5::Declarative
    KF5::GuiAddons
    KF5::I18n
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::NewStuffCore
    KF5::QuickAddons
    KF5::WindowSystem
    krdb
)

if(X11_FOUND)
    target_link_libraries(kcm_colors X11::X11)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(kcm_colors Qt::X11Extras)
    else()
        target_link_libraries(kcm_colors Qt::GuiPrivate)
    endif()
endif()

set(plasma-apply-colorscheme_SRCS
    plasma-apply-colorscheme.cpp
    colorsapplicator.cpp
    colorsmodel.cpp
    ../kcms-common.cpp
    ../krdb/krdb.cpp
)

kconfig_add_kcfg_files(plasma-apply-colorscheme_SRCS colorssettings.kcfgc GENERATE_MOC)

add_executable(plasma-apply-colorscheme ${plasma-apply-colorscheme_SRCS})

target_link_libraries(plasma-apply-colorscheme
    Qt::Core
    Qt::DBus
    Qt::Gui
    KF5::GuiAddons
    KF5::KCMUtils
    KF5::I18n
    KF5::WindowSystem
    PW::KWorkspace
    X11::X11
)
if (QT_MAJOR_VERSION EQUAL "5")
    target_link_libraries(plasma-apply-colorscheme Qt::X11Extras)
else()
    target_link_libraries(plasma-apply-colorscheme Qt::GuiPrivate)
endif()

install(FILES colorssettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES kcm_colors.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(TARGETS plasma-apply-colorscheme DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES colorschemes.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

kpackage_install_package(package kcm_colors kcms)

add_subdirectory(editor)
