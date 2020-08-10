add_subdirectory(kcminit)
add_subdirectory(waitforname)

add_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII)
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

qt5_add_dbus_interface(
    startplasma_SRCS
    ${CMAKE_SOURCE_DIR}/ksplash/ksplashqml/org.kde.KSplash.xml
    ksplashinterface
)

add_executable(startplasma-x11 startplasma.cpp startplasma-x11.cpp kcheckrunning/kcheckrunning.cpp ${startplasma_SRCS})
add_executable(startplasma-wayland startplasma.cpp startplasma-wayland.cpp ${startplasma_SRCS})
add_executable(startplasma-waylandsession startplasma.cpp startplasma-waylandsession.cpp ${startplasma_SRCS})
add_executable(kde-systemd-start-condition kde-systemd-start-condition.cpp)

target_include_directories(startplasma-x11 PRIVATE ${X11_X11_INCLUDE_PATH})
target_link_libraries(startplasma-x11 PRIVATE Qt5::Core Qt5::DBus KF5::ConfigCore PW::KWorkspace
    ${X11_X11_LIB} # for kcheckrunning
)
target_link_libraries(startplasma-wayland PRIVATE Qt5::Core Qt5::DBus KF5::ConfigCore PW::KWorkspace)
target_link_libraries(startplasma-waylandsession PRIVATE Qt5::Core Qt5::DBus KF5::ConfigCore PW::KWorkspace)
target_link_libraries(kde-systemd-start-condition PUBLIC KF5::ConfigCore KF5::Service)
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
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(TARGETS kde-systemd-start-condition DESTINATION ${KDE_INSTALL_BINDIR})
endif ()
