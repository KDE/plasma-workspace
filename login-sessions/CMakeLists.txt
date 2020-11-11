## For Plasma end users

configure_file(plasmax11.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasmax11.desktop)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/plasmax11.desktop
        DESTINATION ${KDE_INSTALL_DATADIR}/xsessions
)

configure_file(plasmawayland.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasmawayland.desktop)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/plasmawayland.desktop
        DESTINATION ${KDE_INSTALL_DATADIR}/wayland-sessions
)

## For Plasma developers
configure_file(startplasma-dev.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/startplasma-dev.sh)
configure_file(plasmax11-dev.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasmax11-dev.desktop)
configure_file(plasmawayland-dev.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasmawayland-dev.desktop)
configure_file(install-sessions.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/install-sessions.sh)
