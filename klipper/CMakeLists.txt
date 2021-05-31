set(KLIPPER_VERSION_STRING ${PROJECT_VERSION})
add_definitions(-DTRANSLATION_DOMAIN=\"klipper\")
add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

add_subdirectory(systemclipboard)

set(libklipper_common_SRCS
    klipper.cpp
    urlgrabber.cpp
    configdialog.cpp
    history.cpp
    historyitem.cpp
    historymodel.cpp
    historystringitem.cpp
    klipperpopup.cpp
    popupproxy.cpp
    historyimageitem.cpp
    historyurlitem.cpp
    actionstreewidget.cpp
    editactiondialog.cpp
    clipcommandprocess.cpp
)

ecm_qt_declare_logging_category(libklipper_common_SRCS HEADER klipper_debug.h IDENTIFIER KLIPPER_LOG CATEGORY_NAME org.kde.klipper DESCRIPTION "klipper" EXPORT KLIPPER)

configure_file(config-klipper.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-klipper.h )

ki18n_wrap_ui(libklipper_common_SRCS generalconfig.ui actionsconfig.ui editactiondialog.ui)
kconfig_add_kcfg_files(libklipper_common_SRCS klippersettings.kcfgc)

add_library(libklipper_common_static STATIC ${libklipper_common_SRCS})
target_link_libraries(libklipper_common_static
    Qt::Concurrent
    KF5::ConfigGui
    KF5::CoreAddons
    KF5::DBusAddons
    KF5::GlobalAccel
    KF5::KIOWidgets
    KF5::Notifications
    KF5::Service
    KF5::Prison
    KF5::TextWidgets
    KF5::WindowSystem
    KF5::WidgetsAddons
    KF5::XmlGui
    ${ZLIB_LIBRARY}
    systemclipboard)
if (X11_FOUND)
    target_link_libraries(libklipper_common_static XCB::XCB Qt::X11Extras)
endif()

add_executable(klipper main.cpp tray.cpp)
target_link_libraries(klipper
    libklipper_common_static
)

install(TARGETS klipper         ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES org.kde.klipper.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES klipper.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

# Plasma Data Engine
set(plasma_engine_clipboard_SRCS ${libklipper_common_SRCS} clipboardengine.cpp clipboardservice.cpp clipboardjob.cpp)
kcoreaddons_add_plugin(plasma_engine_clipboard SOURCES ${plasma_engine_clipboard_SRCS} INSTALL_NAMESPACE "plasma/dataengine")
kcoreaddons_desktop_to_json(plasma_engine_clipboard plasma-dataengine-clipboard.desktop)
target_link_libraries(plasma_engine_clipboard
    Qt::Concurrent
    Qt::DBus
    Qt::Widgets       # QAction
    KF5::ConfigGui
    KF5::CoreAddons    # KUrlMimeData
    KF5::GlobalAccel
    KF5::KIOWidgets    # PreviewJob
    KF5::Plasma
    KF5::Notifications
    KF5::Service
    KF5::Prison
    KF5::TextWidgets   # KTextEdit
    KF5::WidgetsAddons # KMessageBox
    KF5::WindowSystem
    KF5::XmlGui        # KActionCollection
    ${ZLIB_LIBRARY}
    systemclipboard
)
if (X11_FOUND)
  target_link_libraries(plasma_engine_clipboard XCB::XCB Qt::X11Extras)
endif()
install(FILES plasma-dataengine-clipboard.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES org.kde.plasma.clipboard.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

ecm_qt_install_logging_categories(EXPORT KLIPPER FILE klipper.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

