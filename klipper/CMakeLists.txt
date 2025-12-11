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
generate_export_header(klipper)
target_link_libraries(klipper
    Qt::Concurrent
    Qt::DBus
    Qt::Sql
    Qt::Widgets
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::GuiAddons
    KF6::GlobalAccel
    KF6::I18n
    KF6::IconThemes
    KF6::IconWidgets
    KF6::KIOWidgets
    KF6::Notifications
    KF6::Prison
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
    target_link_libraries(klipper XCB::XCB XCB::AUX)
endif()

set_target_properties(klipper PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS klipper ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(declarative)

install(FILES org.kde.klipper.desktop DESTINATION ${KDE_INSTALL_APPDIR})

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

ecm_qt_install_logging_categories(EXPORT KLIPPER FILE klipper.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

add_subdirectory(kconf_update)
