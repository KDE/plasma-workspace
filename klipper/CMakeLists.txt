set(KLIPPER_VERSION_STRING ${PROJECT_VERSION})
add_definitions(-DTRANSLATION_DOMAIN=\"klipper\")

set(libklipper_common_SRCS
    klipper.cpp
    urlgrabber.cpp urlgrabber.h
    configdialog.cpp configdialog.h
    historycycler.cpp historycycler.h
    historyitem.cpp historyitem.h
    historymodel.cpp
    klipperpopup.cpp
    actionstreewidget.cpp
    editactiondialog.cpp
    editcommanddialog.cpp
    clipcommandprocess.cpp
    updateclipboardjob.cpp
    utils.cpp
    systemclipboard.cpp
)

ecm_qt_declare_logging_category(libklipper_common_SRCS HEADER klipper_debug.h IDENTIFIER KLIPPER_LOG CATEGORY_NAME org.kde.klipper DESCRIPTION "klipper" EXPORT KLIPPER)

configure_file(config-klipper.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-klipper.h )

kconfig_add_kcfg_files(libklipper_common_SRCS klippersettings.kcfgc)

add_library(klipper SHARED ${libklipper_common_SRCS})
add_library(PW::LibKlipper ALIAS klipper)

generate_export_header(klipper)

target_include_directories(klipper PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/klipper>")

target_link_libraries(klipper
    PUBLIC
        Qt::Sql
        KF6::GuiAddons
        KF6::GlobalAccel
        KF6::KIOWidgets
    PRIVATE
        Qt::Concurrent
        Qt::DBus
        Qt::Widgets
        KF6::CoreAddons
        KF6::I18n
        KF6::IconThemes
        KF6::IconWidgets
        KF6::Notifications
        KF6::Service
        KF6::WidgetsAddons
        KF6::WindowSystem
        KF6::XmlGui
        Plasma::KWaylandClient
        Plasma::PlasmaQuick
        Wayland::Client
        LayerShellQt::Interface
        ${ZLIB_LIBRARY}
)

if (HAVE_X11)
    target_link_libraries(klipper PRIVATE XCB::XCB XCB::AUX)
endif()

set_target_properties(klipper PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR}
                      EXPORT_NAME LibKlipper)

install(TARGETS klipper EXPORT libklipperLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
        actionstreewidget.h
        clipcommandprocess.h
        configdialog.h
        editactiondialog.h
        editcommanddialog.h
        historycycler.h
        historyitem.h
        historymodel.h
        klipper.h
        klipperpopup.h
        systemclipboard.h
        updateclipboardjob.h
        urlgrabber.h
        utils.h
        ${CMAKE_CURRENT_BINARY_DIR}/klipper_export.h
        ${CMAKE_CURRENT_BINARY_DIR}/config-klipper.h
        DESTINATION ${KDE_INSTALL_INCLUDEDIR}/klipper COMPONENT Devel
)

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibKlipperConfigVersion.cmake VERSION "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion)

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_CMAKEPACKAGEDIR}/LibKlipper)

configure_package_config_file(LibKlipperConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/LibKlipperConfig.cmake"
        INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LibKlipperConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/LibKlipperConfigVersion.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(EXPORT libklipperLibraryTargets
        NAMESPACE PW::
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
        FILE LibKlipperLibraryTargets.cmake )

add_subdirectory(declarative)

install(FILES org.kde.klipper.desktop DESTINATION ${KDE_INSTALL_APPDIR})

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

ecm_qt_install_logging_categories(EXPORT KLIPPER FILE klipper.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

add_subdirectory(kconf_update)
