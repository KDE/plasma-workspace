# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_fonts\")

########### next target ###############

add_library(kcm_fonts_static STATIC kxftconfig.cpp)
target_link_libraries(kcm_fonts_static Qt::Core Qt::Xml KF6::I18n KF6::WindowSystem KF6::KCMUtils KF6::KCMUtilsQuick kfontinst krdb)

set_property(TARGET kcm_fonts_static PROPERTY POSITION_INDEPENDENT_CODE ON)

if(HAVE_X11)
    target_link_libraries(kcm_fonts_static X11::X11 X11::Xft XCB::IMAGE)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(kcm_fonts_static Qt::X11Extras)
    else()
        target_link_libraries(kcm_fonts_static Qt::GuiPrivate)
    endif()
endif()

set(kcm_fonts_PART_SRCS
    previewrenderengine.cpp
    previewimageprovider.cpp
    fonts.cpp
    fontsaasettings.cpp
    fontinit.cpp
    ../kfontinst/lib/FcEngine.cpp
    ../kcms-common.cpp
)

kcmutils_generate_module_data(
    kcm_fonts_PART_SRCS
    MODULE_DATA_HEADER fontsdata.h
    MODULE_DATA_CLASS_NAME FontsData
    SETTINGS_HEADERS fontssettings.h fontsaasettings.h
    SETTINGS_CLASSES FontsSettings FontsAASettings
)

kconfig_add_kcfg_files(kcm_fonts_PART_SRCS fontssettings.kcfgc fontsaasettingsbase.kcfgc GENERATE_MOC)
kcmutils_add_qml_kcm(kcm_fonts SOURCES ${kcm_fonts_PART_SRCS})

target_link_libraries(kcm_fonts Qt::Quick kcm_fonts_static)

########### install files ###############
install(FILES fontssettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

add_custom_command(TARGET kcm_fonts POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ../kcms/systemsettings/kcm_fonts.so kcm_fonts_init.so)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kcm_fonts_init.so DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcminit)

add_subdirectory(autotests)
