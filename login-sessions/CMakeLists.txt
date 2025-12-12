## For Plasma end users

configure_file(plasmawayland.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasma.desktop)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/plasma.desktop
        DESTINATION ${KDE_INSTALL_DATADIR}/wayland-sessions
)

if(WITH_X11_SESSION)
    configure_file(plasmax11.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasmax11.desktop)
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/plasmax11.desktop
            DESTINATION ${KDE_INSTALL_DATADIR}/xsessions
    )
endif()

## For Plasma developers
configure_file(startplasma-dev.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/startplasma-dev.sh)
if(WITH_X11_SESSION)
    configure_file(plasmax11-dev.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasmax11-dev6.desktop)
endif()
configure_file(plasmawayland-dev.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasmawayland-dev6.desktop)
configure_file(install-sessions.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/install-sessions.sh)
