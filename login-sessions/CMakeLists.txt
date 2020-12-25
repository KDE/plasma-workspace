## For Plasma end users

if(PLASMA_WAYLAND_DEFAULT_SESSION)
   set(PLASMA_X11_DESKTOP_FILENAME plasmax11.desktop CACHE INTERNAL "Filename for Plasma X11 session")
   set(PLASMA_WAYLAND_DESKTOP_FILENAME plasma.desktop CACHE INTERNAL "Filename for Plasma Wayland session")
else()
   set(PLASMA_X11_DESKTOP_FILENAME plasma.desktop CACHE INTERNAL "Filename for Plasma X11 session")
   set(PLASMA_WAYLAND_DESKTOP_FILENAME plasmawayland.desktop CACHE INTERNAL "Filename for Plasma Wayland session")
endif()

configure_file(plasmax11.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/${PLASMA_X11_DESKTOP_FILENAME})
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PLASMA_X11_DESKTOP_FILENAME}
        DESTINATION ${KDE_INSTALL_DATADIR}/xsessions
)

configure_file(plasmawayland.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/${PLASMA_WAYLAND_DESKTOP_FILENAME})
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PLASMA_WAYLAND_DESKTOP_FILENAME}
        DESTINATION ${KDE_INSTALL_DATADIR}/wayland-sessions
)

## For Plasma developers
configure_file(startplasma-dev.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/startplasma-dev.sh)
configure_file(plasmax11-dev.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasmax11-dev.desktop)
configure_file(plasmawayland-dev.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasmawayland-dev.desktop)
configure_file(install-sessions.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/install-sessions.sh)
