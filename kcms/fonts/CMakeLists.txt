add_definitions(-DTRANSLATION_DOMAIN=\"kcm_fonts\")

########### next target ###############

add_library(kcm_fonts_static STATIC kxftconfig.cpp)
target_link_libraries(kcm_fonts_static Qt::Core Qt::Xml KF6::I18n KF6::WindowSystem KF6::KCMUtils KF6::KCMUtilsQuick kfontinst krdb)

set_property(TARGET kcm_fonts_static PROPERTY POSITION_INDEPENDENT_CODE ON)

if(HAVE_X11)
    target_link_libraries(kcm_fonts_static X11::X11 X11::Xft XCB::IMAGE Qt::GuiPrivate)
endif()

kcmutils_add_qml_kcm(kcm_fonts)
target_sources(kcm_fonts PRIVATE
    previewrenderengine.cpp
    previewimageprovider.cpp
    fonts.cpp
    fontsaasettings.cpp
    fontinit.cpp
    ../kfontinst/lib/FcEngine.cpp
    ../kcms-common.cpp
)

kcmutils_generate_module_data(kcm_fonts
    MODULE_DATA_HEADER fontsdata.h
    MODULE_DATA_CLASS_NAME FontsData
    SETTINGS_HEADERS fontssettings.h fontsaasettings.h
    SETTINGS_CLASSES FontsSettings FontsAASettings
)
kconfig_add_kcfg_files(kcm_fonts fontssettings.kcfgc fontsaasettingsbase.kcfgc GENERATE_MOC)

target_link_libraries(kcm_fonts PRIVATE Qt::Quick kcm_fonts_static)

########### install files ###############
install(FILES fontssettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

add_custom_command(TARGET kcm_fonts POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ../kcms/systemsettings/kcm_fonts.so kcm_fonts_init.so)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kcm_fonts_init.so DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcminit)

add_subdirectory(kconf_update)

if (BUILD_TESTING)
   add_subdirectory(autotests)
endif()
