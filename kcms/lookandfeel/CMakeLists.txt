add_definitions(-DTRANSLATION_DOMAIN=\"kcm_lookandfeel\")

add_subdirectory(kded)

if(HAVE_X11)
    set(HAVE_XCURSOR TRUE)
endif()

if(Breeze_FOUND)
    if(BREEZE_WITH_KDECORATION)
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
   target_link_libraries(lookandfeelmanager X11::Xcursor X11::Xfixes Qt::GuiPrivate)
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

set_property(TARGET lookandfeelmanager PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(lookandfeelmanager
    Qt::DBus
    Qt::Quick
    KF6::ConfigCore
    KF6::GuiAddons
    KF6::I18n
    KF6::IconThemes
    KF6::KIOGui
    KF6::KCMUtils
    KF6::NewStuffCore
    KF6::Package
    KF6::ColorScheme
    PlasmaAutostart
    krdb
)

kcmutils_add_qml_kcm(kcm_lookandfeel)
target_sources(kcm_lookandfeel PRIVATE
    kcm.cpp
)
target_link_libraries(kcm_lookandfeel PRIVATE
    lookandfeelmanager
    KF6::JobWidgets
    KF6::KIOGui
    KF6::KCMUtilsQuick
    PW::KLookAndFeel
    PW::KWorkspace
)

install(FILES lookandfeelsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES lookandfeel.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

if(BUILD_TESTING)
   find_package(Qt6Test ${QT_MIN_VERSION} CONFIG REQUIRED)
   add_subdirectory(autotests)
endif()

###### Command line tool

add_executable(plasma-apply-lookandfeel lnftool.cpp)
target_link_libraries(plasma-apply-lookandfeel PRIVATE
    lookandfeelmanager
)

ADD_CUSTOM_TARGET(plasma-apply-lookandfeel_link ALL
                  COMMAND ${CMAKE_COMMAND} -E create_symlink ${KDE_INSTALL_FULL_BINDIR}/plasma-apply-lookandfeel ${CMAKE_CURRENT_BINARY_DIR}/lookandfeeltool)
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/lookandfeeltool DESTINATION ${KDE_INSTALL_FULL_BINDIR}/ )
install( TARGETS plasma-apply-lookandfeel ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
