add_definitions(-DTRANSLATION_DOMAIN=\"kcm_autostart\")

if(SYSTEMD_FOUND)
    pkg_check_modules(JOURNALD "libsystemd" REQUIRED)
    add_definitions(-DHAVE_SYSTEMD=1)
endif()

kcmutils_add_qml_kcm(kcm_autostart SOURCES ${kcm_autostart_PART_SRCS})
target_sources(kcm_autostart PRIVATE
   autostartmodel.cpp
   autostart.cpp
   autostartmodel.h
   autostart.h
   unit.cpp
   unit.h)
target_link_libraries(kcm_autostart
    Qt6::Quick
    Qt6::DBus
    KF6::I18n
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::KCMUtilsQuick
    PW::KWorkspace
    ${JOURNALD_LIBRARIES})
ecm_qt_declare_logging_category(kcm_autostart
    HEADER kcm_autostart_debug.h
    IDENTIFIER KCM_AUTOSTART_DEBUG
    CATEGORY_NAME org.kde.plasma.kcm_autostart
    DESCRIPTION "kcm autostart"
    EXPORT PLASMAWORKSPACE
)
