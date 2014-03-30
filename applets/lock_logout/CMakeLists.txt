project(lockout)

install(DIRECTORY contents
        DESTINATION ${DATA_INSTALL_DIR}/plasma/plasmoids/lockout)
install(FILES metadata.desktop
        DESTINATION ${DATA_INSTALL_DIR}/plasma/plasmoids/lockout)

install(FILES metadata.desktop
        DESTINATION ${SERVICES_INSTALL_DIR}
        RENAME plasma-applet-lockout.desktop)
