# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_autostart\")

set(kcm_autostart_PART_SRCS
   autostartmodel.cpp
   autostart.cpp )

kcoreaddons_add_plugin(kcm_autostart SOURCES ${kcm_autostart_PART_SRCS} INSTALL_NAMESPACE "plasma/kcms/systemmsettings")

target_link_libraries(kcm_autostart KF5::I18n KF5::KIOCore KF5::KIOWidgets KF5::QuickAddons PW::KWorkspace)

install(FILES kcm_autostart.desktop DESTINATION ${KDE_INSTALL_APPDIR})
kpackage_install_package(package kcm_autostart kcms)
