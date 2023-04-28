# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_colors\")

kcmutils_add_qml_kcm(kcm_colors)

kcmutils_generate_module_data(kcm_colors
    MODULE_DATA_HEADER colorsdata.h
    MODULE_DATA_CLASS_NAME ColorsData
    SETTINGS_HEADERS colorssettings.h
    SETTINGS_CLASSES ColorsSettings
)
# needed for krdb
kconfig_add_kcfg_files(kcm_colors colorssettings.kcfgc GENERATE_MOC)
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
    target_link_libraries(kcm_colors X11::X11 Qt::GuiPrivate)
endif()

target_sources(kcm_colors PRIVATE
    ../kcms-common.cpp
    colors.cpp
    colorsapplicator.cpp
    colorsmodel.cpp
    filterproxymodel.cpp
)


qt_add_dbus_adaptor(plasma-accentcolor-service_DBUS_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.accentColor.xml
                     accentColorService/accentColorService.h AccentColorService
                     accentcolor_service_adaptor AccentColorServiceAdaptor)


add_executable(plasma-apply-colorscheme)
kconfig_add_kcfg_files(plasma-apply-colorscheme colorssettings.kcfgc GENERATE_MOC)
qt_generate_dbus_interface(
    accentColorService/accentColorService.h
    org.kde.plasmashell.accentColor.xml
    OPTIONS -M
)
message(WARNING "${plasma-accentcolor-service_DBUS_SRCS}")
target_sources(plasma-apply-colorscheme PRIVATE ${plasma-accentcolor-service_DBUS_SRCS})
target_sources(plasma-apply-colorscheme PRIVATE 
    plasma-apply-colorscheme.cpp
    colorsapplicator.cpp
    colorsmodel.cpp
    ../kcms-common.cpp
    accentColorService/accentColorService.cpp
)
target_link_libraries(plasma-apply-colorscheme
    Qt::Core
    Qt::DBus
    Qt::Gui
    Qt::GuiPrivate
    KF6::GuiAddons
    KF6::KCMUtils
    KF6::I18n
    KF6::WindowSystem
    KF6::DBusAddons
    PW::KWorkspace
    X11::X11
    krdb
)

kcoreaddons_add_plugin(plasma_accentcolor_service INSTALL_NAMESPACE "kf6/kded")

kconfig_add_kcfg_files(plasma_accentcolor_service colorssettings.kcfgc GENERATE_MOC)
target_sources(plasma_accentcolor_service PRIVATE
    accentColorService/accentColorService.cpp
    colorsapplicator.cpp
    ../kcms-common.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmashell.accentColor.xml
    ${plasma-accentcolor-service_DBUS_SRCS}
)
target_link_libraries(plasma_accentcolor_service
    Qt::Core
    Qt::DBus
    Qt::GuiPrivate
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

install(FILES colorssettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(TARGETS plasma-apply-colorscheme DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES colorschemes.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

add_subdirectory(editor)
