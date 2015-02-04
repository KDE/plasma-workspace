
install(DIRECTORY common/ DESTINATION ${PLASMA_DATA_INSTALL_DIR}/plasmoids/org.kde.plasma.systemmonitor.net)
plasma_install_package(net org.kde.plasma.systemmonitor.net)

install(DIRECTORY common/ DESTINATION ${PLASMA_DATA_INSTALL_DIR}/plasmoids/org.kde.plasma.systemmonitor.cpu)
plasma_install_package(cpu org.kde.plasma.systemmonitor.cpu)

install(DIRECTORY common/ DESTINATION ${PLASMA_DATA_INSTALL_DIR}/plasmoids/org.kde.plasma.systemmonitor.diskactivity)
plasma_install_package(diskactivity org.kde.plasma.systemmonitor.diskactivity)
