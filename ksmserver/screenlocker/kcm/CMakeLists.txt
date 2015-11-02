# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"screenlocker_kcm\")

set(screenlocker_kcm_SRCS
    kcm.cpp
    selectimagebutton.cpp
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)

ki18n_wrap_ui(screenlocker_kcm_SRCS kcm.ui)
kconfig_add_kcfg_files(screenlocker_kcm_SRCS ../kcfg/kscreensaversettings.kcfgc)
qt5_add_dbus_interface(screenlocker_kcm_SRCS ../dbus/org.kde.screensaver.xml screenlocker_interface)

add_library(screenlocker_kcm MODULE ${screenlocker_kcm_SRCS})

target_link_libraries(screenlocker_kcm
    Qt5::DBus
    KF5::ConfigWidgets
    KF5::I18n
    KF5::GlobalAccel
    KF5::Service
    KF5::Package
    KF5::XmlGui
    Qt5::QuickWidgets
)

kcoreaddons_desktop_to_json(screenlocker_kcm screenlocker.desktop)

install(
    TARGETS
        screenlocker_kcm
    DESTINATION
        ${KDE_INSTALL_PLUGINDIR}
)

install(
    FILES
        screenlocker.desktop
    DESTINATION
        ${KDE_INSTALL_KSERVICES5DIR}
)

plasma_install_package(package screenlocker_kcm kcms screenlocker_kcm)
