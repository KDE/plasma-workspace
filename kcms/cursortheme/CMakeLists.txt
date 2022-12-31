# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_cursortheme\")

set( libnoinst_SRCS
xcursor/thememodel.cpp
xcursor/themeapplicator.cpp
xcursor/cursortheme.cpp
xcursor/xcursortheme.cpp
xcursor/previewwidget.cpp
xcursor/sortproxymodel.cpp
../kcms-common.cpp
)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/xcursor/ )

########### next target ###############

set(kcm_cursortheme_PART_SRCS kcmcursortheme.cpp ${libnoinst_SRCS})

kcmutils_generate_module_data(
    kcm_cursortheme_PART_SRCS
    MODULE_DATA_HEADER cursorthemedata.h
    MODULE_DATA_CLASS_NAME CursorThemeData
    SETTINGS_HEADERS cursorthemesettings.h launchfeedbacksettings.h
    SETTINGS_CLASSES CursorThemeSettings LaunchFeedbackSettings
)


kconfig_add_kcfg_files(kcm_cursortheme_PART_SRCS cursorthemesettings.kcfgc GENERATE_MOC)
kconfig_add_kcfg_files(kcm_cursortheme_PART_SRCS launchfeedbacksettings.kcfgc GENERATE_MOC)
kcoreaddons_add_plugin(kcm_cursortheme SOURCES ${kcm_cursortheme_PART_SRCS} INSTALL_NAMESPACE "plasma/kcms/systemsettings")
kcmutils_generate_desktop_file (kcm_cursortheme)


target_link_libraries(kcm_cursortheme
    Qt::DBus
    Qt::Quick
    KF5::Archive
    KF5::KCMUtils
    KF5::I18n
    KF5::GuiAddons
    KF5::WindowSystem
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::NewStuffCore
    KF5::QuickAddons
    PW::KWorkspace
    krdb
)

if (HAVE_X11)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(kcm_cursortheme Qt::X11Extras)
    else()
        target_link_libraries(kcm_cursortheme Qt::GuiPrivate)
    endif()

   target_link_libraries(kcm_cursortheme X11::Xcursor X11::Xfixes)
endif ()

########### next target ###############

set(plasma-apply-cursortheme_SRCS
    plasma-apply-cursortheme.cpp

    xcursor/cursortheme.cpp
    xcursor/themeapplicator.cpp
    xcursor/thememodel.cpp
    xcursor/xcursortheme.cpp
    ../kcms-common.cpp
    ../krdb/krdb.cpp
)

kconfig_add_kcfg_files(plasma-apply-cursortheme_SRCS cursorthemesettings.kcfgc GENERATE_MOC)
add_executable(plasma-apply-cursortheme ${plasma-apply-cursortheme_SRCS})

target_link_libraries(plasma-apply-cursortheme
    Qt::DBus
    KF5::GuiAddons
    KF5::I18n
    KF5::KCMUtils
    KF5::WindowSystem
    PW::KWorkspace
)

if (HAVE_X11)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(plasma-apply-cursortheme Qt::X11Extras)
    else()
        target_link_libraries(plasma-apply-cursortheme Qt::GuiPrivate)
    endif()

   target_link_libraries(plasma-apply-cursortheme X11::X11 X11::Xcursor X11::Xfixes)
endif()

install(TARGETS plasma-apply-cursortheme  DESTINATION ${KDE_INSTALL_BINDIR})

########### install files ###############

install(FILES cursorthemesettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES launchfeedbacksettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES delete_cursor_old_default_size.upd delete_cursor_old_default_size.pl DESTINATION ${KDE_INSTALL_DATADIR}/kconf_update)
install( FILES xcursor/xcursor.knsrc DESTINATION  ${KDE_INSTALL_KNSRCDIR} )

kpackage_install_package(package kcm_cursortheme kcms)

