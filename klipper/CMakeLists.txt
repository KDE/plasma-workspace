set(KLIPPER_VERSION_STRING ${PROJECT_VERSION})
add_definitions(-DTRANSLATION_DOMAIN=\"klipper\")

set(libklipper_common_SRCS
    klipper.cpp
    urlgrabber.cpp
    configdialog.cpp
    historycycler.cpp
    historyitem.cpp
    historymodel.cpp
    historystringitem.cpp
    klipperpopup.cpp
    historyimageitem.cpp
    historyurlitem.cpp
    actionstreewidget.cpp
    editactiondialog.cpp
    editcommanddialog.cpp
    clipcommandprocess.cpp
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
    target_link_libraries(klipper Qt::GuiPrivate XCB::XCB)
endif()

set_target_properties(klipper PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS klipper ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(declarative)

install(FILES org.kde.klipper.desktop DESTINATION ${KDE_INSTALL_APPDIR})

# Plasma Data Engine
set(plasma_engine_clipboard_SRCS clipboardengine.cpp clipboardservice.cpp clipboardjob.cpp)
kcoreaddons_add_plugin(plasma_engine_clipboard SOURCES ${plasma_engine_clipboard_SRCS} INSTALL_NAMESPACE "plasma5support/dataengine")
target_link_libraries(plasma_engine_clipboard
    klipper
    Plasma::Plasma5Support
)

install(FILES org.kde.plasma.clipboard.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

ecm_qt_install_logging_categories(EXPORT KLIPPER FILE klipper.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

