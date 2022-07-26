# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_lookandfeel\")

if(${Breeze_FOUND})
    if(${BREEZE_WITH_KDECORATION})
        set(HAVE_BREEZE_DECO TRUE)
    endif()
endif()

configure_file (config-kcm.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcm.h )

add_library(lookandfeelmanager STATIC
    lookandfeelmanager.cpp
    ../colors/colorsapplicator.cpp
    ../kcms-common.cpp
)

if (HAVE_X11)
    target_sources(lookandfeelmanager PRIVATE
        ../cursortheme/xcursor/cursortheme.cpp
        ../cursortheme/xcursor/xcursortheme.cpp
    )
endif ()

kcmutils_generate_module_data(
    lookandfeeldata
    MODULE_DATA_HEADER lookandfeeldata.h
    MODULE_DATA_CLASS_NAME LookAndFeelData
    SETTINGS_HEADERS lookandfeelsettings.h
    SETTINGS_CLASSES LookAndFeelSettings
)
target_sources(lookandfeelmanager PRIVATE ${lookandfeeldata})
kconfig_add_kcfg_files(lookandfeelmanager lookandfeelsettings.kcfgc GENERATE_MOC)

target_link_libraries(lookandfeelmanager
    Qt::DBus
    KF5::ConfigCore
    KF5::GuiAddons
    KF5::I18n
    KF5::KIOGui
    KF5::KCMUtils
    KF5::NewStuff
    KF5::Package
    PlasmaAutostart
    krdb
)

if(HAVE_X11)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(lookandfeelmanager Qt::X11Extras)
    else()
        target_link_libraries(lookandfeelmanager Qt::GuiPrivate)
    endif()
   target_link_libraries(lookandfeelmanager X11::Xcursor X11::Xfixes)
endif()

add_library(kcm_lookandfeel_objects OBJECT kcm.cpp)
target_link_libraries(kcm_lookandfeel_objects
    lookandfeelmanager
    KF5::IconThemes
    KF5::JobWidgets
    KF5::KIOGui
    KF5::QuickAddons
    PW::KWorkspace
)

kcoreaddons_add_plugin(kcm_lookandfeel SOURCES kcmmain.cpp INSTALL_NAMESPACE "plasma/kcms/systemsettings")
kcmutils_generate_desktop_file(kcm_lookandfeel)
target_link_libraries(kcm_lookandfeel kcm_lookandfeel_objects)

install(FILES lookandfeelsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES lookandfeel.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

if(BUILD_TESTING)
   find_package(Qt${QT_MAJOR_VERSION}Test ${QT_MIN_VERSION} CONFIG REQUIRED)
   add_subdirectory(autotests)
endif()

kpackage_install_package(package kcm_lookandfeel kcms)

###### Command line tool

add_executable(plasma-apply-lookandfeel lnftool.cpp)

target_link_libraries(plasma-apply-lookandfeel kcm_lookandfeel_objects)

ADD_CUSTOM_TARGET(plasma-apply-lookandfeel_link ALL
                  COMMAND ${CMAKE_COMMAND} -E create_symlink ${KDE_INSTALL_FULL_BINDIR}/plasma-apply-lookandfeel ${CMAKE_CURRENT_BINARY_DIR}/lookandfeeltool)
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/lookandfeeltool DESTINATION ${KDE_INSTALL_FULL_BINDIR}/ )
install( TARGETS plasma-apply-lookandfeel ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
