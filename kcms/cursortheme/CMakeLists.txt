add_definitions(-DTRANSLATION_DOMAIN=\"kcm_cursortheme\")

add_library(kcm_cursortheme_sources OBJECT)
set_target_properties(kcm_cursortheme_sources PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(kcm_cursortheme_sources PRIVATE
    xcursor/thememodel.cpp
    xcursor/themeapplicator.cpp
    xcursor/cursortheme.cpp
    xcursor/xcursortheme.cpp
    xcursor/previewwidget.cpp
    xcursor/sortproxymodel.cpp
    ../kcms-common.cpp
    kcmcursortheme.cpp
)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/xcursor/ ${CMAKE_CURRENT_BINARY_DIR}/../krdb/ )

kcmutils_generate_module_data(kcm_cursortheme_sources
    MODULE_DATA_HEADER cursorthemedata.h
    MODULE_DATA_CLASS_NAME CursorThemeData
    SETTINGS_HEADERS cursorthemesettings.h launchfeedbacksettings.h
    SETTINGS_CLASSES CursorThemeSettings LaunchFeedbackSettings
)


kconfig_add_kcfg_files(kcm_cursortheme_sources cursorthemesettings.kcfgc GENERATE_MOC)
kconfig_add_kcfg_files(kcm_cursortheme_sources launchfeedbacksettings.kcfgc GENERATE_MOC)

target_link_libraries(kcm_cursortheme_sources PUBLIC
    Qt::DBus
    Qt::Quick
    KF6::Archive
    KF6::KCMUtils
    KF6::DBusAddons
    KF6::I18n
    KF6::GuiAddons
    KF6::WindowSystem
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::NewStuffCore
    KF6::KCMUtilsQuick
    krdb
)

if (HAVE_X11)
   target_link_libraries(kcm_cursortheme_sources PRIVATE X11::Xcursor X11::Xfixes Qt::GuiPrivate)
endif ()

add_library(qml STATIC)
ecm_add_qml_module(qml URI "org.kde.private.kcm_cursortheme" VERSION 1.0 GENERATE_PLUGIN_SOURCE PLUGIN_TARGET qmlplugin)
set_target_properties(qml PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(qml PRIVATE kcm_cursortheme_sources)
qt_generate_foreign_qml_types(kcm_cursortheme_sources qml)

kcmutils_add_qml_kcm(kcm_cursortheme)
target_link_libraries(kcm_cursortheme PRIVATE kcm_cursortheme_sources qml qmlplugin)

# plasma-apply-cursortheme utility

add_executable(plasma-apply-cursortheme)
target_sources(plasma-apply-cursortheme PRIVATE
    plasma-apply-cursortheme.cpp

    xcursor/cursortheme.cpp
    xcursor/themeapplicator.cpp
    xcursor/thememodel.cpp
    xcursor/xcursortheme.cpp
    ../kcms-common.cpp
    ../krdb/krdb.cpp
)
kconfig_add_kcfg_files(plasma-apply-cursortheme cursorthemesettings.kcfgc GENERATE_MOC)
target_link_libraries(plasma-apply-cursortheme
    Qt::DBus
    KF6::DBusAddons
    KF6::GuiAddons
    KF6::I18n
    KF6::KCMUtils
    KF6::WindowSystem
)

if (HAVE_X11)
   target_link_libraries(plasma-apply-cursortheme X11::X11 X11::Xcursor X11::Xfixes XCB::XCB XCB::CURSOR Qt::GuiPrivate KF6::ColorScheme)
endif()

install(TARGETS plasma-apply-cursortheme  DESTINATION ${KDE_INSTALL_BINDIR})

########### install files ###############

install(FILES cursorthemesettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES launchfeedbacksettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install( FILES xcursor/xcursor.knsrc DESTINATION  ${KDE_INSTALL_KNSRCDIR} )

