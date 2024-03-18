add_subdirectory(plasmaautostart)
add_subdirectory(kcminit)
add_subdirectory(waitforname)
add_subdirectory(startup-sound)

if (SYSTEMD_FOUND)
    add_subdirectory(systemd)
endif()

qt_add_dbus_interface(
    startplasma_SRCS
    ${CMAKE_SOURCE_DIR}/ksplash/ksplashqml/org.kde.KSplash.xml
    ksplashinterface
)
ecm_qt_declare_logging_category(startplasma_SRCS HEADER debug.h IDENTIFIER PLASMA_STARTUP CATEGORY_NAME org.kde.startup)

add_library(startplasma OBJECT startplasma.cpp ${startplasma_SRCS})
target_link_libraries(startplasma PUBLIC
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

kconfig_target_kcfg_file(startplasma
    FILE ${CMAKE_SOURCE_DIR}/kcms/lookandfeel/lookandfeelsettings.kcfg
    CLASS_NAME LookAndFeelSettings
)

add_executable(startplasma-wayland ${START_PLASMA_COMMON_SRCS} startplasma-wayland.cpp)

target_link_libraries(startplasma-wayland PRIVATE
    startplasma
)

if (WITH_X11_SESSION)
    add_executable(startplasma-x11 ${START_PLASMA_COMMON_SRCS} startplasma-x11.cpp kcheckrunning/kcheckrunning.cpp)

    target_link_libraries(startplasma-x11 PRIVATE
        startplasma
        X11::X11 # for kcheckrunning
    )
endif()

add_subdirectory(plasma-session)
add_subdirectory(plasma-shutdown)
add_subdirectory(session-shortcuts)
add_subdirectory(session-restore)

#FIXME: reconsider, looks fishy
if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    set_property(SOURCE startplasma.cpp APPEND PROPERTY COMPILE_DEFINITIONS
        XCURSOR_PATH="~/.local/share/icons:~/.icons:${KDE_INSTALL_FULL_DATAROOTDIR}/icons:$XCURSOR_PATH:/usr/share/icons:/usr/share/pixmaps:/usr/X11R6/lib/X11/icons")
endif()

configure_file(config-startplasma.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-startplasma.h)

install(TARGETS startplasma-wayland ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
if (WITH_X11_SESSION)
    install(TARGETS startplasma-x11 ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
endif()
install(PROGRAMS plasma-sourceenv.sh DESTINATION ${KDE_INSTALL_LIBEXECDIR})
install(PROGRAMS plasma-dbus-run-session-if-needed DESTINATION ${KDE_INSTALL_LIBEXECDIR})

add_executable(plasma-setup-live-system plasma-setup-live-system.cpp)
target_link_libraries(plasma-setup-live-system PRIVATE KF6::ConfigCore)
install(TARGETS plasma-setup-live-system DESTINATION ${KDE_INSTALL_LIBEXECDIR})
ecm_install_configured_files(INPUT plasma-setup-live-system.service.in DESTINATION ${KDE_INSTALL_SYSTEMDUNITDIR}/system)

add_executable(plasma-setup-live-user plasma-setup-live-user.cpp)
target_link_libraries(plasma-setup-live-user PRIVATE KF6::ConfigCore)
install(TARGETS plasma-setup-live-user DESTINATION ${KDE_INSTALL_LIBEXECDIR})
ecm_install_configured_files(INPUT plasma-setup-live-user.service.in DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})

add_subdirectory(autotests)
