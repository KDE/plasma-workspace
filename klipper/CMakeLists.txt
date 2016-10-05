set(KLIPPER_VERSION_STRING ${PROJECT_VERSION})
add_definitions(-DTRANSLATION_DOMAIN=\"klipper\")

set(libklipper_common_SRCS
    klipper_debug.cpp
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

find_package(KF5Prison 5.25)
set_package_properties(KF5Prison PROPERTIES DESCRIPTION "Prison library"
                       URL "http://projects.kde.org/prison"
                       TYPE OPTIONAL
                       PURPOSE "Needed to create mobile barcodes from clipboard data (no stable release currently)"
                      )

set(HAVE_PRISON ${KF5Prison_FOUND})
configure_file(config-klipper.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-klipper.h )

kde4_add_app_icon(libklipper_common_SRCS "${KDE4_ICON_INSTALL_DIR}/oxygen/*/apps/klipper.png")
ki18n_wrap_ui(libklipper_common_SRCS generalconfig.ui actionsconfig.ui editactiondialog.ui)
kconfig_add_kcfg_files(libklipper_common_SRCS klippersettings.kcfgc)

set(klipper_KDEINIT_SRCS ${libklipper_common_SRCS} main.cpp tray.cpp)


kf5_add_kdeinit_executable(klipper ${klipper_KDEINIT_SRCS})

target_link_libraries(kdeinit_klipper
    Qt5::Concurrent
    KF5::Completion # klineedit - port away?
    KF5::ConfigGui
    KF5::CoreAddons
    KF5::DBusAddons
    KF5::GlobalAccel
    KF5::IconThemes
    KF5::Notifications
    KF5::Service
    KF5::TextWidgets
    KF5::WindowSystem
    KF5::WidgetsAddons
    KF5::XmlGui
    ${ZLIB_LIBRARY}
)
if (X11_FOUND)
  target_link_libraries(kdeinit_klipper XCB::XCB Qt5::X11Extras)
endif()
if (HAVE_PRISON)
  target_link_libraries(kdeinit_klipper KF5::Prison)
endif ()

install(TARGETS kdeinit_klipper ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS klipper         ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS org.kde.klipper.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(PROGRAMS klipper.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

# Plasma Data Engine
set(plasma_engine_clipboard_SRCS ${libklipper_common_SRCS} clipboardengine.cpp clipboardservice.cpp clipboardjob.cpp)
add_library(plasma_engine_clipboard MODULE ${plasma_engine_clipboard_SRCS})
kcoreaddons_desktop_to_json(plasma_engine_clipboard plasma-dataengine-clipboard.desktop)
target_link_libraries(plasma_engine_clipboard
    Qt5::Concurrent
    Qt5::DBus
    Qt5::Widgets       # QAction
    KF5::ConfigGui
    KF5::Completion    # klineedit - port away?
    KF5::CoreAddons    # KUrlMimeData
    KF5::GlobalAccel
    KF5::IconThemes
    KF5::KIOWidgets    # PreviewJob
    KF5::Plasma
    KF5::Notifications
    KF5::Service
    KF5::TextWidgets   # KTextEdit
    KF5::WidgetsAddons # KMessageBox
    KF5::WindowSystem
    KF5::XmlGui        # KActionCollection
    ${ZLIB_LIBRARY}
)
if (X11_FOUND)
  target_link_libraries(plasma_engine_clipboard XCB::XCB Qt5::X11Extras)
endif()
if (HAVE_PRISON)
  target_link_libraries(plasma_engine_clipboard KF5::Prison)
endif ()
install(TARGETS plasma_engine_clipboard DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-clipboard.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES org.kde.plasma.clipboard.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)

add_subdirectory(autotests)
