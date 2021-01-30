add_subdirectory(kcminit)
add_subdirectory(waitforname)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory(systemd)
endif()

add_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII)
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

qt5_add_dbus_interface(
    startplasma_SRCS
    ${CMAKE_SOURCE_DIR}/ksplash/ksplashqml/org.kde.KSplash.xml
    ksplashinterface
)


set(START_PLASMA_COMMON_SRCS startplasma.cpp)

ecm_qt_declare_logging_category(START_PLASMA_COMMON_SRCS HEADER debug.h IDENTIFIER PLASMA_STARTUP CATEGORY_NAME org.kde.startup)

add_executable(startplasma-x11 ${START_PLASMA_COMMON_SRCS} startplasma-x11.cpp kcheckrunning/kcheckrunning.cpp ${startplasma_SRCS})
add_executable(startplasma-wayland ${START_PLASMA_COMMON_SRCS} startplasma-wayland.cpp ${startplasma_SRCS})
add_executable(startplasma-waylandsession ${START_PLASMA_COMMON_SRCS} startplasma-waylandsession.cpp ${startplasma_SRCS})

target_link_libraries(startplasma-x11 PRIVATE Qt::Core Qt::DBus KF5::ConfigCore PW::KWorkspace
    X11::X11 # for kcheckrunning
)

target_link_libraries(startplasma-wayland PRIVATE Qt::Core Qt::DBus KF5::ConfigCore PW::KWorkspace)
target_link_libraries(startplasma-waylandsession PRIVATE Qt::Core Qt::DBus KF5::ConfigCore PW::KWorkspace)
add_subdirectory(plasma-session)
add_subdirectory(plasma-shutdown)

#FIXME: reconsider, looks fishy
if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    set_property(SOURCE startplasma.cpp APPEND PROPERTY COMPILE_DEFINITIONS
        XCURSOR_PATH="${KDE_INSTALL_FULL_DATAROOTDIR}/icons:$XCURSOR_PATH:~/.icons:/usr/share/icons:/usr/share/pixmaps:/usr/X11R6/lib/X11/icons")
endif()

configure_file(config-startplasma.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-startplasma.h)

install(TARGETS startplasma-x11 ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS startplasma-wayland ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS startplasma-waylandsession DESTINATION ${KDE_INSTALL_LIBEXECDIR})
install(PROGRAMS plasma-sourceenv.sh DESTINATION ${KDE_INSTALL_LIBEXECDIR})
install(PROGRAMS plasma-dbus-run-session-if-needed DESTINATION ${KDE_INSTALL_LIBEXECDIR})
