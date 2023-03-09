# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_autostart\")

set(kcm_autostart_PART_SRCS
   autostartmodel.cpp
   autostart.cpp
   autostartmodel.h
   autostart.h)

kcoreaddons_add_plugin(kcm_autostart SOURCES ${kcm_autostart_PART_SRCS} INSTALL_NAMESPACE "plasma/kcms/systemsettings")
kcmutils_generate_desktop_file (kcm_autostart)

target_link_libraries(kcm_autostart Qt6::Quick KF6::I18n KF6::KIOCore KF6::KIOWidgets KF6::KCMUtilsQuick PW::KWorkspace)

ecm_qt_declare_logging_category(kcm_autostart
    HEADER kcm_autostart_debug.h
    IDENTIFIER KCM_AUTOSTART_DEBUG
    CATEGORY_NAME org.kde.plasma.kcm_autostart
)

kpackage_install_package(package kcm_autostart kcms)
