## For Plasma end users

configure_file(plasmawayland.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasma.desktop)
configure_file(plasmawayland-safemode.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasma-safemode.desktop)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/plasma.desktop
        DESTINATION ${KDE_INSTALL_DATADIR}/wayland-sessions
)

configure_file(startplasma-safemode.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/startplasma-safemode.sh)
if (INSTALL_SAFEMODE_SESSION) # experimental for now
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/startplasma-safemode.sh DESTINATION ${KDE_INSTALL_LIBEXECDIR})
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/plasma-safemode.desktop
            DESTINATION ${KDE_INSTALL_DATADIR}/wayland-sessions
    )
    install(DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}/plasma-safemode-template
            DESTINATION ${KDE_INSTALL_DATADIR})
endif()

## For Plasma developers
configure_file(startplasma-dev.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/startplasma-dev.sh)
configure_file(plasmawayland-dev.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasmawayland-dev6.desktop)
configure_file(install-sessions.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/install-sessions.sh)
