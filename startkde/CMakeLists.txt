add_subdirectory(plasmaautostart)
add_subdirectory(waitforname)
add_subdirectory(startup-sound)

if (SYSTEMD_FOUND)
    add_subdirectory(systemd)
endif()

ecm_qt_declare_logging_category(startplasma_SRCS HEADER debug.h IDENTIFIER PLASMA_STARTUP CATEGORY_NAME org.kde.startup)

add_executable(startplasma-wayland startplasma.cpp ${startplasma_SRCS})
target_link_libraries(startplasma-wayland PRIVATE
    Qt::Core
    Qt::DBus
    KF6::ConfigGui
    KF6::DBusAddons
    KF6::Notifications
    KF6::Package
    PW::KLookAndFeel
    PW::KWorkspace
    KNightTime
)

kconfig_target_kcfg_file(startplasma-wayland
    FILE ${CMAKE_SOURCE_DIR}/kcms/lookandfeel/lookandfeelsettings.kcfg
    CLASS_NAME LookAndFeelSettings
)

add_subdirectory(plasma-session)
add_subdirectory(plasma-shutdown)
add_subdirectory(plasma-setup-xwayland)
add_subdirectory(session-shortcuts)
add_subdirectory(session-restore)

#FIXME: reconsider, looks fishy
if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    set_property(SOURCE startplasma.cpp APPEND PROPERTY COMPILE_DEFINITIONS
        XCURSOR_PATH="~/.local/share/icons:~/.icons:${KDE_INSTALL_FULL_DATAROOTDIR}/icons:$XCURSOR_PATH:/usr/share/icons:/usr/share/pixmaps:/usr/X11R6/lib/X11/icons")
endif()

configure_file(config-startplasma.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-startplasma.h)

install(TARGETS startplasma-wayland ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS plasma-sourceenv.sh DESTINATION ${KDE_INSTALL_LIBEXECDIR})
install(PROGRAMS plasma-dbus-run-session-if-needed DESTINATION ${KDE_INSTALL_LIBEXECDIR})
