add_subdirectory(kcminit)
add_subdirectory(kstartupconfig)
add_subdirectory(ksyncdbusenv)
add_subdirectory(waitforname)

qt5_add_dbus_interface(
    startplasma_SRCS
    ${CMAKE_SOURCE_DIR}/ksplash/ksplashqml/org.kde.KSplash.xml
    ksplashinterface
)

add_executable(startplasma-x11 startkde.cpp kcheckrunning/kcheckrunning.cpp kstartupconfig/kstartupconfig.cpp ${startplasma_SRCS})

#for kcheckrunning
target_link_libraries(startplasma-x11 PRIVATE Qt5::Core Qt5::DBus
    KF5::ConfigCore
    X11::X11 # for kcheckrunning
)

#FIXME: reconsider, looks fishy
if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    target_compile_definitions(startplasma-x11 PRIVATE
        -DXCURSOR_PATH="${KDE_INSTALL_FULL_DATAROOTDIR}/icons:$XCURSOR_PATH:~/.icons:/usr/share/icons:/usr/share/pixmaps:/usr/X11R6/lib/X11/icons"
    )
endif()
target_compile_definitions(startplasma-x11 PRIVATE
    -DCMAKE_INSTALL_FULL_BINDIR="${CMAKE_INSTALL_FULL_BINDIR}"
    -DKDE_INSTALL_FULL_DATAROOTDIR="${KDE_INSTALL_FULL_DATAROOTDIR}"
    -DCMAKE_INSTALL_FULL_LIBEXECDIR="${CMAKE_INSTALL_FULL_LIBEXECDIR}"
    -DCMAKE_INSTALL_FULL_LIBEXECDIR_KF5="${CMAKE_INSTALL_FULL_LIBEXECDIR_KF5}"
)

configure_file(startplasmacompositor.cmake ${CMAKE_CURRENT_BINARY_DIR}/startplasmacompositor  @ONLY)
configure_file(startplasma.cmake ${CMAKE_CURRENT_BINARY_DIR}/startplasma  @ONLY)

if(NOT WIN32)
    install(TARGETS startplasma-x11 ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
    install(PROGRAMS plasma-sourceenv.sh DESTINATION ${KDE_INSTALL_LIBEXECDIR})

    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/startkde DESTINATION ${KDE_INSTALL_BINDIR})
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/startplasmacompositor DESTINATION ${KDE_INSTALL_BINDIR})
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/startplasma DESTINATION ${KDE_INSTALL_LIBEXECDIR})
endif()
