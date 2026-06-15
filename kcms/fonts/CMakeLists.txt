add_definitions(-DTRANSLATION_DOMAIN=\"kcm_fonts\")

########### next target ###############

add_library(kcm_fonts_static STATIC kxftconfig.cpp)
target_link_libraries(kcm_fonts_static Qt::Core Qt::Xml KF6::I18n KF6::WindowSystem KF6::KCMUtils KF6::KCMUtilsQuick kfontinst)

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
    ../kfontinst/lib/FcEngine.cpp
    ../kcms-common.cpp
    devicepixelratiohelper.cpp
)

kcmutils_generate_module_data(kcm_fonts
    MODULE_DATA_HEADER fontsdata.h
    MODULE_DATA_CLASS_NAME FontsData
    SETTINGS_HEADERS fontssettings.h fontsaasettings.h
    SETTINGS_CLASSES FontsSettings FontsAASettings
)
kconfig_add_kcfg_files(kcm_fonts fontssettings.kcfgc fontsaasettingsbase.kcfgc GENERATE_MOC)

target_link_libraries(kcm_fonts PRIVATE Qt::Quick kcm_fonts_static)
target_compile_definitions(kcm_fonts PRIVATE CMAKE_INSTALL_FULL_LIBEXECDIR="${CMAKE_INSTALL_FULL_LIBEXECDIR}")

########### install files ###############
install(FILES fontssettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

add_subdirectory(kconf_update)

if (BUILD_TESTING)
   add_subdirectory(autotests)
endif()
