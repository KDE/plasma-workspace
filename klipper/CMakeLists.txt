set(KLIPPER_VERSION_STRING ${PROJECT_VERSION})
add_definitions(-DTRANSLATION_DOMAIN=\"klipper\")
add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

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
    editcommanddialog.cpp
    clipcommandprocess.cpp
    utils.cpp
)

ecm_qt_declare_logging_category(libklipper_common_SRCS HEADER klipper_debug.h IDENTIFIER KLIPPER_LOG CATEGORY_NAME org.kde.klipper DESCRIPTION "klipper" EXPORT KLIPPER)

configure_file(config-klipper.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-klipper.h )

kconfig_add_kcfg_files(libklipper_common_SRCS klippersettings.kcfgc)

add_library(libklipper_common_static STATIC ${libklipper_common_SRCS})
set_property(TARGET libklipper_common_static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(libklipper_common_static
    Qt::Concurrent
    Qt::Widgets
    Qt::DBus
    KF5::ConfigGui
    KF5::CoreAddons
    KF5::DBusAddons
    KF5::GuiAddons
    KF5::GlobalAccel
    KF5::KIOWidgets
    KF5::Notifications
    KF5::Service
    KF5::Prison
    KF5::TextWidgets
    KF5::WindowSystem
    KF5::WidgetsAddons
    KF5::XmlGui
    KF5::WaylandClient
    KF5::IconThemes
    ${ZLIB_LIBRARY})

if (HAVE_X11)
    target_link_libraries(libklipper_common_static XCB::XCB)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(libklipper_common_static Qt::X11Extras)
    else()
        target_link_libraries(libklipper_common_static Qt::GuiPrivate)
    endif()
endif()

add_executable(klipper main.cpp tray.cpp)
target_link_libraries(klipper
    libklipper_common_static
)

install(TARGETS klipper         ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES org.kde.klipper.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES klipper.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

# Plasma Data Engine
set(plasma_engine_clipboard_SRCS clipboardengine.cpp clipboardservice.cpp clipboardjob.cpp)
kcoreaddons_add_plugin(plasma_engine_clipboard SOURCES ${plasma_engine_clipboard_SRCS} INSTALL_NAMESPACE "plasma/dataengine")
target_link_libraries(plasma_engine_clipboard
    libklipper_common_static
    KF5::Plasma
)

install(FILES org.kde.plasma.clipboard.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

ecm_qt_install_logging_categories(EXPORT KLIPPER FILE klipper.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

