# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_colors\")

set(kcm_colors_SRCS
    ../kcms-common.cpp
    colors.cpp
    colorsapplicator.cpp
    colorsmodel.cpp
    filterproxymodel.cpp
)

qt_generate_dbus_interface(
    accentColorService/accentColorService.h
    org.kde.plasmashell.accentColor.xml
    OPTIONS -M
)

set(plasma-accentcolor-service_SRCS
    accentColorService/accentColorService.cpp
    colorsapplicator.cpp
    ../kcms-common.cpp
)


set(plasma-accentcolor-service_SRCS
    ${plasma-accentcolor-service_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.accentColor.xml
)

qt_add_dbus_adaptor(plasma-accentcolor-service_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.accentColor.xml
                     accentColorService/accentColorService.h AccentColorService
                     accentcolor_service_adaptor AccentColorServiceAdaptor)

kcmutils_generate_module_data(
    kcm_colors_SRCS
    MODULE_DATA_HEADER colorsdata.h
    MODULE_DATA_CLASS_NAME ColorsData
    SETTINGS_HEADERS colorssettings.h
    SETTINGS_CLASSES ColorsSettings
)

# needed for krdb
kconfig_add_kcfg_files(kcm_colors_SRCS colorssettings.kcfgc GENERATE_MOC)

kcmutils_add_qml_kcm(kcm_colors SOURCES ${kcm_colors_SRCS})
target_link_libraries(kcm_colors
    Qt::DBus
    Qt::Quick
    KF6::KCMUtils
    KF6::CoreAddons
    KF6::GuiAddons
    KF6::I18n
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::NewStuffCore
    KF6::KCMUtilsQuick
    KF6::WindowSystem
    krdb
)

if(HAVE_X11)
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
)

kconfig_add_kcfg_files(plasma-apply-colorscheme_SRCS colorssettings.kcfgc GENERATE_MOC)
kconfig_add_kcfg_files(plasma-accentcolor-service_SRCS colorssettings.kcfgc GENERATE_MOC)

add_executable(plasma-apply-colorscheme ${plasma-apply-colorscheme_SRCS})
kcoreaddons_add_plugin(plasma_accentcolor_service SOURCES ${plasma-accentcolor-service_SRCS} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kded")

target_link_libraries(plasma-apply-colorscheme
    Qt::Core
    Qt::DBus
    Qt::Gui
    KF6::GuiAddons
    KF6::KCMUtils
    KF6::I18n
    KF6::WindowSystem
    PW::KWorkspace
    X11::X11
    krdb
)

target_link_libraries(plasma_accentcolor_service
    Qt::Core
    Qt::DBus
    KF6::GuiAddons
    KF6::KCMUtils
    KF6::I18n
    KF6::WindowSystem
    KF6::ConfigCore
    KF6::DBusAddons
    PW::KWorkspace
    X11::X11
    krdb
)


if (QT_MAJOR_VERSION EQUAL "5")
    target_link_libraries(plasma-apply-colorscheme Qt::X11Extras)
else()
    target_link_libraries(plasma-apply-colorscheme Qt::GuiPrivate)
endif()

if (QT_MAJOR_VERSION EQUAL "5")
    target_link_libraries(plasma_accentcolor_service Qt::X11Extras)
else()
    target_link_libraries(plasma_accentcolor_service Qt::GuiPrivate)
endif()

install(FILES colorssettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(TARGETS plasma-apply-colorscheme DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES colorschemes.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

add_subdirectory(editor)
