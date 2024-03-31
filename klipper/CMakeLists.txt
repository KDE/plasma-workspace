set(KLIPPER_VERSION_STRING ${PROJECT_VERSION})
add_definitions(-DTRANSLATION_DOMAIN=\"klipper\")
add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

set(klipper_common_SRCS
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

ecm_qt_declare_logging_category(klipper_common_SRCS HEADER klipper_debug.h IDENTIFIER KLIPPER_LOG CATEGORY_NAME org.kde.klipper DESCRIPTION "klipper" EXPORT KLIPPER)

configure_file(config-klipper.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-klipper.h )

kconfig_add_kcfg_files(klipper_common_SRCS klippersettings.kcfgc)

add_library(klipper_common SHARED ${klipper_common_SRCS})
generate_export_header(klipper_common)
set_target_properties(klipper_common PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_link_libraries(klipper_common
    PUBLIC
        Qt::Core
        Qt::Qml
    PRIVATE
        Qt::Concurrent
        Qt::Widgets
        Qt::DBus
        KF6::ConfigGui
        KF6::CoreAddons
        KF6::DBusAddons
        KF6::GuiAddons
        KF6::GlobalAccel
        KF6::KIOWidgets
        KF6::Notifications
        KF6::Service
        KF6::Prison
        KF6::TextWidgets
        KF6::WindowSystem
        KF6::WidgetsAddons
        KF6::XmlGui
        Plasma::KWaylandClient
        KF6::IconThemes
        KF6::IconWidgets
        ${ZLIB_LIBRARY}
        Wayland::Client
)

if (HAVE_X11)
    target_link_libraries(klipper_common PRIVATE XCB::XCB)
    target_link_libraries(klipper_common PRIVATE Qt::GuiPrivate)
endif()

ecm_add_qml_module(klipper_common URI org.kde.plasma.private.klipper GENERATE_PLUGIN_SOURCE)
ecm_finalize_qml_module(klipper_common)

if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

ecm_qt_install_logging_categories(EXPORT KLIPPER FILE klipper.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

