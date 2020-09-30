# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_lookandfeel\")

if(X11_Xcursor_FOUND)
    set(HAVE_XCURSOR TRUE)
endif()

if(${Breeze_FOUND})
    if(${BREEZE_WITH_KDECORATION})
        set(HAVE_BREEZE_DECO TRUE)
    endif()
endif()

configure_file (config-kcm.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcm.h )

set(kcm_lookandfeel_SRCS
    kcm.cpp
    ../colors/colorsapplicator.cpp
    ../kcms-common.cpp
)

if (X11_Xcursor_FOUND)
    set(kcm_lookandfeel_SRCS
        ${kcm_lookandfeel_SRCS}
        ../cursortheme/xcursor/cursortheme.cpp
        ../cursortheme/xcursor/xcursortheme.cpp
    )
endif ()

kcmutils_generate_module_data(
    kcm_lookandfeel_SRCS
    MODULE_DATA_HEADER lookandfeeldata.h
    MODULE_DATA_CLASS_NAME LookAndFeelData
    SETTINGS_HEADERS lookandfeelsettings.h
    SETTINGS_CLASSES LookAndFeelSettings
)
kconfig_add_kcfg_files(kcm_lookandfeel_SRCS lookandfeelsettings.kcfgc GENERATE_MOC)

add_library(kcm_lookandfeel_static STATIC ${kcm_lookandfeel_SRCS})
target_link_libraries(kcm_lookandfeel_static
    Qt::DBus
    KF5::I18n
    KF5::KCMUtils
    KF5::IconThemes
    KF5::JobWidgets
    KF5::Service
    KF5::QuickAddons
    KF5::GuiAddons
    KF5::KIOWidgets
    KF5::Declarative
    KF5::NewStuff
    PW::KWorkspace
    krdb
    PlasmaAutostart
)
if(X11_FOUND)
    target_link_libraries(kcm_lookandfeel_static Qt::X11Extras)
endif()
if (X11_Xcursor_FOUND)
   target_link_libraries(kcm_lookandfeel_static X11::Xcursor)
endif ()
if (X11_Xfixes_FOUND)
   target_link_libraries(kcm_lookandfeel_static X11::Xfixes)
endif ()

kcoreaddons_add_plugin(kcm_lookandfeel SOURCES kcmmain.cpp INSTALL_NAMESPACE "kcms")
kcoreaddons_desktop_to_json(kcm_lookandfeel "kcm_lookandfeel.desktop" SERVICE_TYPES kcmodule.desktop)
target_link_libraries(kcm_lookandfeel kcm_lookandfeel_static)

#this desktop file is installed only for retrocompatibility with sycoca
install(FILES lookandfeelsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES kcm_lookandfeel.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES lookandfeel.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

if(BUILD_TESTING)
   find_package(Qt5Test ${QT_MIN_VERSION} CONFIG REQUIRED)
   add_subdirectory(autotests)
endif()

kpackage_install_package(package kcm_lookandfeel kcms)

###### Command line tool

kconfig_add_kcfg_files(plasma-apply-lookandfeel_SRCS lookandfeelsettings.kcfgc GENERATE_MOC)

add_executable(plasma-apply-lookandfeel lnftool.cpp)

target_link_libraries(plasma-apply-lookandfeel kcm_lookandfeel_static)

ADD_CUSTOM_TARGET(plasma-apply-lookandfeel_link ALL
                  COMMAND ${CMAKE_COMMAND} -E create_symlink ${KDE_INSTALL_FULL_BINDIR}/plasma-apply-lookandfeel ${CMAKE_CURRENT_BINARY_DIR}/lookandfeeltool)
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/lookandfeeltool DESTINATION ${KDE_INSTALL_FULL_BINDIR}/ )
install( TARGETS plasma-apply-lookandfeel ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
